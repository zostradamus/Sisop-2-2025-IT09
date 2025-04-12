#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>

void write_log(const char *proc_name, const char *status) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[100];
    strftime(timebuf, sizeof(timebuf), "%d:%m:%Y-%H:%M:%S", t);

    int fd = open("/tmp/debugmon.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        dprintf(fd, "[%s]_%s_%s\n", timebuf, proc_name, status);
        close(fd);
    }
}

int is_user_blocked(const char *user) {
    char deny_path[256];
    snprintf(deny_path, sizeof(deny_path), "/tmp/debugmon_deny_%s", user);
    return access(deny_path, F_OK) == 0;
}

void run_daemon(const char *user) {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    if (setsid() < 0) exit(EXIT_FAILURE);
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    FILE *pid_file = fopen("/tmp/debugmon.pid", "w");
    if (pid_file) {
        fprintf(pid_file, "%d", getpid());
        fclose(pid_file);
    }

    while (1) {
        DIR *dir = opendir("/proc");
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                int pid = atoi(entry->d_name);
                if (pid <= 0) continue;
                char path[256];
                snprintf(path, sizeof(path), "/proc/%d/status", pid);
                FILE *fp = fopen(path, "r");
                if (!fp) continue;
                char line[256];
                char proc_name[256] = "Unknown";

                while (fgets(line, sizeof(line), fp)) {
                    if (strncmp(line, "Uid:", 4) == 0) {
                        uid_t uid;
                        sscanf(line, "Uid:\t%u", &uid);
                        struct passwd *pw = getpwuid(uid);
                        if (pw && strcmp(pw->pw_name, user) == 0) {
                            snprintf(proc_name, sizeof(proc_name), "%s", entry->d_name);
                            write_log(proc_name, "Running");
                        }
                        break;
                    }
                }
                fclose(fp);
            }
        }
        closedir(dir);
        sleep(5);
    }
}

void stop_daemon(const char *user) {
    FILE *pid_file = fopen("/tmp/debugmon.pid", "r");
    if (pid_file) {
        int pid;
        fscanf(pid_file, "%d", &pid);
        fclose(pid_file);
        kill(pid, SIGTERM);
        unlink("/tmp/debugmon.pid");
    }
    write_log("stop", "RUNNING");
}

void list_process(const char *user) {
    if (is_user_blocked(user)) {
        printf("User %s sedang dalam mode blokir.\n", user);
        return;
    }
    pid_t pid = fork();
    if (pid == 0) {
        execlp("ps", "ps", "-u", user, "-o", "pid,comm,%cpu,%mem", NULL);
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

void fail_user(const char *user) {
    write_log("fail", "FAILED");

    char deny_path[256];
    snprintf(deny_path, sizeof(deny_path), "/tmp/debugmon_deny_%s", user);
    int fd = open(deny_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create deny file");
        return;
    }
    close(fd);

    char bashrc_path[256];
    snprintf(bashrc_path, sizeof(bashrc_path), "/home/%s/.bashrc", user);

    FILE *src = fopen(bashrc_path, "r");
    if (!src) {
        perror("Failed to open original .bashrc");
        return;
    }
    FILE *dest = fopen("/tmp/tmp_bashrc", "w");
    if (!dest) {
        perror("Failed to create temporary bashrc file");
        fclose(src);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), src)) {
        fputs(line, dest);
    }
    fclose(src);
    fclose(dest);

    FILE *tmp_bashrc = fopen("/tmp/tmp_bashrc", "r");
    if (!tmp_bashrc) {
        perror("Failed to open temporary bashrc file");
        return;
    }

    FILE *bashrc = fopen(bashrc_path, "w");
    if (!bashrc) {
        perror("Failed to open .bashrc file");
        fclose(tmp_bashrc);
        return;
    }

    while (fgets(line, sizeof(line), tmp_bashrc)) {
        fputs(line, bashrc);
    }

    fprintf(bashrc, "\n# BLOCKED BY DEBUGMON\n");
    fprintf(bashrc, "if [ -f /tmp/debugmon_deny_%s ]; then\n", user);
    fprintf(bashrc, "  echo 'Akses diblokir oleh debugmon.'\n");
    fprintf(bashrc, "  logout\n");
    fprintf(bashrc, "fi\n");
    fprintf(bashrc, "# END BLOCK\n");

    fclose(tmp_bashrc);
    fclose(bashrc);

    if (unlink("/tmp/tmp_bashrc") < 0) {
        perror("Failed to remove temporary bashrc file");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) exit(EXIT_SUCCESS);

    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Failed to open /proc directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            int pid = atoi(entry->d_name);
            if (pid <= 0) continue;
            char path[256];
            snprintf(path, sizeof(path), "/proc/%d/status", pid);
            FILE *fp = fopen(path, "r");
            if (!fp) continue;
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "Uid:", 4) == 0) {
                    uid_t uid;
                    sscanf(line, "Uid:\t%u", &uid);
                    struct passwd *pw = getpwuid(uid);
                    if (pw && strcmp(pw->pw_name, user) == 0) {
                        kill(pid, SIGKILL);
                    }
                    break;
                }
            }
            fclose(fp);
        }
    }
    closedir(dir);
}

void revert_user(const char *user) {
    // Hapus deny file
    char deny_path[256];
    snprintf(deny_path, sizeof(deny_path), "/tmp/debugmon_deny_%s", user);
    if (unlink(deny_path) == 0) {
        printf("Akses untuk user %s telah dipulihkan.\n", user);
    } else {
        perror("Gagal menghapus deny file");
    }

    // Hapus baris blokir di .bashrc
    char bashrc_path[256];
    snprintf(bashrc_path, sizeof(bashrc_path), "/home/%s/.bashrc", user);

    FILE *src = fopen(bashrc_path, "r");
    if (!src) {
        perror("Gagal membuka .bashrc untuk dibaca");
        return;
    }

    FILE *tmp = fopen("/tmp/tmp_bashrc_revert", "w");
    if (!tmp) {
        perror("Gagal membuat file sementara");
        fclose(src);
        return;
    }

    char line[256];
    int skip = 0;
    while (fgets(line, sizeof(line), src)) {
        if (strstr(line, "# BLOCKED BY DEBUGMON")) {
            skip = 1;
            continue;
        }
        if (strstr(line, "# END BLOCK")) {
            skip = 0;
            continue;
        }
        if (!skip) fputs(line, tmp);
    }

    fclose(src);
    fclose(tmp);

    // Salin hasil ke bashrc
    tmp = fopen("/tmp/tmp_bashrc_revert", "r");
    FILE *dest = fopen(bashrc_path, "w");
    if (!tmp || !dest) {
        perror("Gagal membuka file saat mengembalikan bashrc");
        if (tmp) fclose(tmp);
        if (dest) fclose(dest);
        return;
    }

    while (fgets(line, sizeof(line), tmp)) {
        fputs(line, dest);
    }

    fclose(tmp);
    fclose(dest);
    unlink("/tmp/tmp_bashrc_revert");

    write_log("revert", "RUNNING");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <command> <user>\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];
    const char *user = argv[2];

    if (strcmp(command, "list") == 0) {
        list_process(user);
    } else if (strcmp(command, "daemon") == 0) {
        run_daemon(user);
        write_log("daemon", "RUNNING");
    } else if (strcmp(command, "stop") == 0) {
        stop_daemon(user);
    } else if (strcmp(command, "fail") == 0) {
        fail_user(user);
    } else if (strcmp(argv[1], "revert") == 0 && argc == 3) {
        revert_user(argv[2]);
    } else {
        printf("Perintah tidak dikenali: %s\n", command);
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

// Fungsi log sederhana
void log_activity(const char *fmt, ...) {
    FILE *fp = fopen("activity.log", "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    fprintf(fp, "[%02d-%02d-%d][%02d:%02d:%02d] - ",
        tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900,
        tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);

    fprintf(fp, "\n");
    fclose(fp);
}

// Download ZIP dan ekstrak manual
void download_and_extract() {
    const char *url = "https://drive.usercontent.google.com/download?id=1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS&confirm=t";
    const char *zipname = "starterkit.zip";
    const char *folder = "starter_kit";

    pid_t pid = fork();
    if (pid == 0) {
        execlp("wget", "wget", "-O", zipname, url, NULL);
        exit(1);
    } else {
        wait(NULL);
    }

    struct stat st = {0};
    if (stat(folder, &st) == -1) mkdir(folder, 0700);

    pid = fork();
    if (pid == 0) {
        execlp("unzip", "unzip", "-q", zipname, "-d", folder, NULL);
        exit(1);
    } else {
        wait(NULL);
    }

    remove(zipname);
    printf(" Download dan ekstrak selesai!\n");
    log_activity("Starterkit berhasil diunduh dan diekstrak.");
}

// Cek string base64
int is_base64(const char *s) {
    while (*s) {
        if (!(isalnum(*s) || *s == '+' || *s == '/' || *s == '='))
            return 0;
        s++;
    }
    return 1;
}

// Decode base64 pakai pipe
char *decode_b64(const char *text) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "echo '%s' | base64 -d", text);
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    char *res = malloc(256);
    if (!res) return NULL;
    if (!fgets(res, 256, fp)) {
        free(res);
        pclose(fp);
        return NULL;
    }

    res[strcspn(res, "\n")] = '\0';
    pclose(fp);
    return res;
}

// Pindah file ke quarantine
void quarantine_files() {
    struct stat st = {0};
    if (stat("quarantine", &st) == -1) mkdir("quarantine", 0700);

    DIR *dir = opendir("starter_kit");
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG) {
            char src[512], dst[512];
            snprintf(src, sizeof(src), "starter_kit/%s", entry->d_name);
            snprintf(dst, sizeof(dst), "quarantine/%s", entry->d_name);
            if (rename(src, dst) == 0) {
                log_activity("%s dipindah ke quarantine.", entry->d_name);
            }
        }
    }
    closedir(dir);
    printf("File dipindah ke quarantine.\n");
}

// Decrypt daemon
void start_decrypt_daemon() {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) {
        FILE *fp = fopen("daemon.pid", "w");
        if (fp) {
            fprintf(fp, "%d\n", pid);
            fclose(fp);
        }

 	printf("Daemon dekripsi dijalankan (PID %d)\n", pid);

        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "Daemon dekripsi dijalankan (PID %d)", pid);
        log_activity(log_msg);
        exit(0);
    }

    setsid();
    chdir(".");
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    while (1) {
        DIR *dir = opendir("quarantine");
        if (!dir) {
            sleep(5);
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir))) {
            if (entry->d_type == DT_REG && is_base64(entry->d_name)) {
                char path_old[512], path_new[512];
                snprintf(path_old, sizeof(path_old), "quarantine/%s", entry->d_name);
                char *decoded = decode_b64(entry->d_name);
                if (decoded) {
                    snprintf(path_new, sizeof(path_new), "quarantine/%s", decoded);
                    rename(path_old, path_new);
                    free(decoded);
                }
            }
        }

        closedir(dir);
        sleep(5);
    }
}

// Kembalikan file dari quarantine
void restore_files() {
    DIR *dir = opendir("quarantine");
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG) {
            char src[512], dst[512];
            snprintf(src, sizeof(src), "quarantine/%s", entry->d_name);
            snprintf(dst, sizeof(dst), "starter_kit/%s", entry->d_name);
            if (rename(src, dst) == 0) {
                log_activity("%s dikembalikan ke starter_kit.", entry->d_name);
            }
        }
    }
    closedir(dir);
    printf("File dikembalikan.\n");
}

// Hapus file di quarantine
void remove_quarantine() {
    DIR *dir = opendir("quarantine");
    if (!dir) return;

    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG) {
            char path[512];
            snprintf(path, sizeof(path), "quarantine/%s", entry->d_name);
            if (remove(path) == 0) {
                log_activity("%s dihapus dari quarantine.", entry->d_name);
                count++;
            }
        }
    }
    closedir(dir);
    printf("%d file dihapus dari quarantine.\n", count);
}

// Hentikan daemon
void stop_daemon() {
    FILE *fp = fopen("daemon.pid", "r");
    if (!fp) {
        printf("daemon.pid tidak ditemukan.\n");
        return;
    }

    pid_t pid;
    if (fscanf(fp, "%d", &pid) != 1) {
        fclose(fp);
        return;
    }
    fclose(fp);

    if (kill(pid, SIGTERM) == 0) {
        log_activity("Daemon (PID %d) dihentikan.", pid);
        remove("daemon.pid");
        printf("Daemon dihentikan.\n");
    } else {
        printf("Gagal menghentikan daemon.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Gunakan argumen: --download | --decrypt | --quarantine | --return | --eradicate | --shutdown\n");
        return 1;
    }

    if (strcmp(argv[1], "--download") == 0) {
        download_and_extract();
    } else if (strcmp(argv[1], "--decrypt") == 0) {
        start_decrypt_daemon();
    } else if (strcmp(argv[1], "--quarantine") == 0) {
        quarantine_files();
    } else if (strcmp(argv[1], "--return") == 0) {
        restore_files();
    } else if (strcmp(argv[1], "--eradicate") == 0) {
        remove_quarantine();
    } else if (strcmp(argv[1], "--shutdown") == 0) {
        stop_daemon();
    } else {
        printf(" Argumen tidak dikenal.\n");
    }

    return 0;
}


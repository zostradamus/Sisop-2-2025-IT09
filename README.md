# LAPORAN RESMI SOAL MODUL 2 SISOP

## ANGGOTA KELOMPOK
| Nama                           | NRP        |
| -------------------------------| ---------- |
| Shinta Alya Ramadani           | 5027241016 |
| Prabaswara Febrian Winandika   | 5027241069 |
| Muhammad Farrel Rafli Al Fasya | 5027241075 |

### SOAL 1

### SOAL 2
#### Analisis Soal
Pada soal ini, dibuat sebuah program starterkit.c yang dapat mengelola file dari hasil unduhan ZIP secara terstruktur dan aman, dengan fitur :
 - Download dan ekstrak file ZIP,
 - Menjalankan daemon untuk mendekripsi nama file terenkripsi base64,
 - Melakukan karantina file,
 - Mengembalikan file dari karantina,
 - Menghapus file dari karantina,
 - Menghentikan proses daemon,
 - Serta mencatat semua aktivitas ke dalam file log (activity.log).
Semua proses dilakukan menggunakan sistem call fork, exec, dan lainnya tanpa menggunakan system(), dan ditulis dalam bahasa C.

#### Analisis Kode
##### Fungsi Log Activity
```sh
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
```
Fungsi ini bertugas mencatat log ke dalam activity.log dengan format timestamp [dd-mm-yyyy][hh:mm:ss]. Digunakan untuk melacak semua tindakan penting selama program berjalan.

##### Fungsi Download dan Extract
```sh
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
```
Fungsi ini :
 - Mengunduh file ZIP dari link Google Drive menggunakan wget,
 - Mengekstrak isinya ke folder starter_kit menggunakan unzip,
 - Menghapus file ZIP,
 - Mencatat log keberhasilan ekstraksi.
OUTPUT :

##### Fungsi is_base64
```sh
int is_base64(const char *s) {
    while (*s) {
        if (!(isalnum(*s) || *s == '+' || *s == '/' || *s == '='))
            return 0;
        s++;
    }
    return 1;
}
```
Fungsi validasi untuk mengecek apakah nama file adalah representasi base64 yang valid, digunakan oleh daemon untuk identifikasi file terenkripsi.

##### Fungsi decode_b64
```sh
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
```
Mendekripsi string base64 menggunakan pipe (popen) dan perintah base64 -d. Hasilnya akan digunakan untuk mengganti nama file ke nama aslinya.

##### Fungsi Quarantine
```sh
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
               log_activity("%s - Successfully moved to quarantine directory.", entry->d_name);
            }
        }
    }
    closedir(dir);
    printf("File dipindah ke quarantine.\n");
}
```
File dalam folder starter_kit akan dipindah ke folder quarantine. Setiap pemindahan akan dicatat di log.
OUTPUT :
```sh
File dipindah ke quarantine.
```

##### Fungsi Decrypt
```sh
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
        log_activity("Successfully started decryption process with PID %d.", pid);
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
```
Daemon berjalan di background dan setiap 5 detik :
 - Mengecek file di folder quarantine,
 - Jika nama file terenkripsi base64, maka didekripsi dan diganti namanya,
 - PID disimpan dalam daemon.pid dan log dicatat.
OUTPUT :
```sh
[17-04-2025][16:42:10] - Daemon dekripsi dijalankan (PID 12453)
```

##### Fungsi Return
```sh
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
               log_activity("%s - Successfully returned to starter kit directory.", entry->d_name);
            }
        }
    }
    closedir(dir);
    printf("File dikembalikan.\n");
}
```
Memindahkan kembali file dari folder quarantine ke starter_kit. Aksi ini juga dicatat di log.
OUTPUT :
```sh
File dikembalikan.
```

##### Fungsi Eradicate
```sh
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
                log_activity("%s - Successfully deleted.", entry->d_name);
                count++;
            }
        }
    }
    closedir(dir);
    printf("%d file dihapus dari quarantine.\n", count);
}
```
Memindahkan kembali file dari folder quarantine ke starter_kit. Aksi ini juga dicatat di log.
OUTPUT :
```sh
3 file dihapus dari quarantine.
```

##### Fungsi Shutdown
```sh
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
        log_activity("Successfully shut off decryption process with PID %d.", pid);
        remove("daemon.pid");
        printf("Daemon dihentikan.\n");
    } else {
        printf("Gagal menghentikan daemon.\n");
    }
}
```
Menghentikan proses daemon dengan membaca PID dari file daemon.pid dan mengirimkan sinyal SIGTERM. Jika berhasil, log dicatat dan file PID dihapus.
OUTPUT :
```sh
Daemon dihentikan.
```

##### Fungsi Main
```sh
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
```
###### REVISI
- Error muncul saat mencoba menjalankam dua argumen sekaligus:
  ```sh
  Contoh : ./starterkit --quarantine --decrypt
  ```
- Revisi :
  Menambahkan error handling di main() untuk mencegah argumen lebih dari satu.
  SEBELUM REVISI
  ```sh
  if (argc < 2) {
    printf("Gunakan argumen: --download | --decrypt | --quarantine | --return | --eradicate | --shutdown\n");
    return 1;
  }
  ```
  SETELAH REVISI
  ```sh
  if (argc != 2) {
    printf("Error: Hanya satu argumen yang diperbolehkan.\n");
    printf("Gunakan salah satu dari:\n");
    printf("  --download | --decrypt | --quarantine | --return | --eradicate | --shutdown\n");
    return 1;
  }
  ```
  OUTPUT :
  ```sh
  Error: Hanya satu argumen yang diperbolehkan.
  Gunakan salah satu dari:
  --download | --decrypt | --quarantine | --return | --eradicate | --shutdown
  ```
Program menerima perintah berbasis argumen sebagai berikut :
|     Argumen      |                	Fungsi                |
| -----------------| -------------------------------------- |
|   --download	   |  Mengunduh dan mengekstrak ZIP         |
|   --decrypt	     |  Menjalankan daemon dekripsi nama file |
|   --quarantine	 |  Memindahkan file ke folder quarantine |
|   --return	     |  Mengembalikan file dari quarantine    |
|   --eradicate	   |  Menghapus semua file dalam quarantine |
|   --shutdown	   |  Menghentikan daemon dekripsi          |

### SOAL 3

### SOAL 4

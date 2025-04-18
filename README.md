# LAPORAN RESMI SOAL MODUL 2 SISOP

## ANGGOTA KELOMPOK
| Nama                           | NRP        |
| -------------------------------| ---------- |
| Shinta Alya Ramadani           | 5027241016 |
| Prabaswara Febrian Winandika   | 5027241069 |
| Muhammad Farrel Rafli Al Fasya | 5027241075 |

### SOAL 1
# Tugas Praktikum Sistem Operasi - action.c

## Deskripsi
Program `action.c` dibuat untuk memproses file petunjuk (Clues) yang diberikan melalui link download. Program ini memiliki beberapa mode operasi, serta dapat mengunduh dan mengekstrak file `.zip` secara otomatis saat dijalankan tanpa argumen.

---

## Fitur dan Cuplikan Kode

### 🔽 1. Download dan Ekstraksi Otomatis

Saat program dijalankan tanpa argumen (`./action`), ia akan memanggil fungsi `download_and_extract()`:

```c
void download_and_extract() {
    struct stat st = {0};
    if (stat("Clues", &st) == -1) {
        printf("Folder Clues tidak ditemukan. Mendownload Clues.zip...\n");

        CURL *curl = curl_easy_init();
        if (curl) {
            FILE *fp = fopen("Clues.zip", "wb");
            curl_easy_setopt(curl, CURLOPT_URL, "https://drive.usercontent.google.com/u/0/uc?id=1xFn1OBJUuSdnApDseEczKhtNzyGekauK&export=download");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);
        }

        int err = 0;
        zip_t *zip = zip_open("Clues.zip", 0, &err);
        if (zip) {
            zip_int64_t num_entries = zip_get_num_entries(zip, 0);
            for (zip_int64_t i = 0; i < num_entries; i++) {
                const char *name = zip_get_name(zip, i, 0);
                struct zip_stat st;
                zip_stat_init(&st);
                zip_stat(zip, name, 0, &st);

                zip_file_t *zf = zip_fopen(zip, name, 0);
                if (zf) {
                    char out_path[256];
                    snprintf(out_path, sizeof(out_path), "Clues/%s", name);
                    char *last_slash = strrchr(out_path, '/');
                    if (last_slash) {
                        *last_slash = '\0';
                        mkdir(out_path, 0755);
                        *last_slash = '/';
                    }
                    FILE *fout = fopen(out_path, "wb");
                    if (fout) {
                        char buffer[1024];
                        zip_int64_t bytes_read;
                        while ((bytes_read = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
                            fwrite(buffer, 1, bytes_read, fout);
                        }
                        fclose(fout);
                    }
                    zip_fclose(zf);
                }
            }
            zip_close(zip);
            remove("Clues.zip");
            printf("Selesai download dan ekstrak Clues.zip\n");
        } else {
            printf("Gagal membuka Clues.zip\n");
        }
    } else {
        printf("Folder Clues sudah ada. Melewati proses download.\n");
    }
}
```

---

### 🧹 2. Filter File Valid

```bash
./action -m Filter
```

Fungsi `filter_files()` akan memindahkan file `.txt` dari folder `Clues/ClueA` s.d `ClueD` ke folder `Filtered/`, hanya jika nama file 1 karakter alfanumerik:

```c
void filter_files() {
    char dirName[100];
    char fileName[50];
    char destName[100];
    DIR *d;
    struct dirent *dir;

    mkdir("Filtered", 0777); // Buat folder Filtered kalau belum ada

    for (char c = 'A'; c <= 'D'; c++) {
        snprintf(dirName, sizeof(dirName), "Clues/Clue%c", c);
        d = opendir(dirName);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                    continue;

                char name[20];
                strncpy(name, dir->d_name, strlen(dir->d_name) - 4);
                name[strlen(dir->d_name) - 4] = '\0';

                if (strlen(name) == 1 && isalnum(name[0])) {
                    snprintf(fileName, sizeof(fileName), "%s/%s", dirName, dir->d_name);
                    snprintf(destName, sizeof(destName), "Filtered/%s", dir->d_name);
                    printf("Memindahkan file valid: %s\n", fileName);
                    rename(fileName, destName);
                }
            }
            closedir(d);
        } else {
            printf("Gagal membuka direktori %s\n", dirName);
        }
    }
}
```

---

### 📦 3. Gabung File

```bash
./action -m Combine
```

Gabungkan file dari `Filtered/` ke `Combined.txt`:

```c
void combine_files() {
    DIR *d = opendir("Filtered");
    if (!d) {
        printf("Folder 'Filtered' tidak ditemukan!\n");
        return;
    }

    FILE *out = fopen("Combined.txt", "w");
    if (!out) {
        perror("Gagal membuat Combined.txt");
        return;
    }

    char digits[100][20], alphas[100][20];
    int digit_count = 0, alpha_count = 0;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (is_digit_file(dir->d_name)) {
            strcpy(digits[digit_count++], dir->d_name);
        } else if (is_alpha_file(dir->d_name)) {
            strcpy(alphas[alpha_count++], dir->d_name);
        }
    }
    closedir(d);

    qsort(digits, digit_count, sizeof(digits[0]), compare_strings);
    qsort(alphas, alpha_count, sizeof(alphas[0]), compare_strings);

    int i = 0, j = 0;
    while (i < digit_count || j < alpha_count) {
        if (i < digit_count) {
            char path[100];
            sprintf(path, "Filtered/%s", digits[i++]);
            FILE *fp = fopen(path, "r");
            if (fp) {
                char c;
                while ((c = fgetc(fp)) != EOF) fputc(c, out);
                fclose(fp);
            }
        }
        if (j < alpha_count) {
            char path[100];
            sprintf(path, "Filtered/%s", alphas[j++]);
            FILE *fp = fopen(path, "r");
            if (fp) {
                char c;
                while ((c = fgetc(fp)) != EOF) fputc(c, out);
                fclose(fp);
            }
        }
    }

    fclose(out);
    printf("Combine selesai. Hasil di Combined.txt\n");
}
```

---

### 🔓 4. Decode ROT13

```bash
./action -m Decode
```

Mendekripsi isi `Combined.txt` menggunakan ROT13:

```c
char rot13(char c) {
    if ('a' <= c && c <= 'z') return (c - 'a' + 13) % 26 + 'a';
    if ('A' <= c && c <= 'Z') return (c - 'A' + 13) % 26 + 'A';
    return c;
}

void decode_file() {
    FILE *in = fopen("Combined.txt", "r");
    FILE *out = fopen("Decoded.txt", "w");
    if (!in || !out) {
        printf("Combined.txt tidak ditemukan!\n");
        return;
    }

    char c;
    while ((c = fgetc(in)) != EOF) {
        fputc(rot13(c), out);
    }

    fclose(in);
    fclose(out);
    printf("Decoded selesai. Hasil di Decoded.txt\n");
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}
```

---

## Kompilasi

```bash
gcc action.c -o action -lcurl -lzip
```

---

## Dependensi

```bash
sudo apt install libcurl4-openssl-dev libzip-dev
```

---

## Perbandingan Versi

| Fitur             | Versi Lama                     | Versi Revisi                                       |
|------------------|-------------------------------|---------------------------------------------------|
| Filter File       | Error        | Sekarang Aman           |
| Download ZIP      | Tidak ada                      | Ditambahkan dengan `libcurl`                      |
| Ekstrak ZIP       | Tidak ada                      | Ditambahkan dengan `libzip`                       |

---

## Struktur Output

```
├── Clues/
│   ├── ClueA/
│   ├── ClueB/
├── Filtered/
│   ├── a.txt
│   ├── 1.txt
├── Combined.txt
├── Decoded.txt
```

---

## Catatan
- Program **tidak menggunakan `system()`**.
- Portabel dan aman digunakan di berbagai distro Linux.
- Jalankan tanpa argumen untuk download dan ekstrak otomatis.



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
#### Analisis Soal 
Suatu hari, Nobita menemukan sebuah alat aneh di laci mejanya. Alat ini berbentuk robot kecil yang bernama debugmon "Robot super kepo yang bisa memantau semua aktivitas di komputer". Agar debugon bisa memantau, mengakses, dan mencatat berbagai proses di komputer, dibuatlah fitur-fitur seperti:
1. Mengetahui semua aktivitas user (./debugmon list <user>)
2. Memasang mata-mata dalam mode daemon dan mencatat semua proses user di dalam file.log (./debugmon daemon <user>)
3. Menghentikan pemantauan daemon (./debugmon stop <user>)
4. Menggagalkan semua proses user yang sedang berjalan, lalu menulis status FAILED dalam file.log, dan memblokir user untuk menjalankan proses (./debugmon fail <user>)
5. Mengizinkan user untuk kembali menjalankan proses dan mengembalikan ke mode normal sebelum di fail (./debugmon revert <user>)
6. Mencatat semua hasil run dari fitur-fitur tadi ke dalam file.log (debugmon.log) dengan format [dd:mm:yyyy]-[hh:mm:ss]_nama-process_STATUS(RUNNING/FAILED), untuk fitur daemon, stop dan revert status prosesnya adalah RUNNING, sedangkan untuk fitur fail status prosesnya adalah FAILED
#### Analisis Code 
##### **Fungsi write_log**
###### Menulis log ke /tmp/debugmon.log dengan timestamp, nama proses, dan status.
```sh
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
```
###### Output write_log di /tmp/debugmon.log
![dokumentasi write_log](https://github.com/user-attachments/assets/359ce373-f95a-4243-898d-8a049d5375e8)
##### **Fungsi is_user_blocked**
###### Mengecek apakah file blokir user (/tmp/debugmon_deny_<user>) ada
```sh
int is_user_blocked(const char *user) {
    char deny_path[256];
    snprintf(deny_path, sizeof(deny_path), "/tmp/debugmon_deny_%s", user);
    return access(deny_path, F_OK) == 0;
}
```
##### **Fungsi list_process (./debugmon list <kali>)**
###### Jika user tidak diblokir, menampilkan proses milik user dengan ps
```sh
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
```
###### Output:
![dokumentasi list_process](https://github.com/user-attachments/assets/ba7dbb9b-4ab9-4a68-8d2d-34b9aa016810)
##### **Fungsi run_daemon (./debugmon daemon <user>)**
###### Menjalankan daemon yang mencatat proses milik user tertentu setiap 5 detik ke log
```sh
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
```
###### Bukti ./debugmon daemon <user> telah berjalan
![dokumentasi run_daemon](https://github.com/user-attachments/assets/f8c1e5e3-387a-4d75-b7d2-79716fc8ff08)
###### Output hasil pencatatan run_daemon di debugmon.log
![dokumentasi run_daemon_2](https://github.com/user-attachments/assets/42ffe435-2422-4fa3-a954-9f4bc415c483)
###### PID dari proses daemon (debugmon.pid)
![dokumentasi run_daemon_3](https://github.com/user-attachments/assets/b99bb0a3-329d-40d5-96a7-a9d64e288ecc)
##### **Fungsi stop_daemon (./debugmon stop <user>)**
###### Membaca PID dari file, lalu mengirim SIGTERM ke daemon
```sh
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
```
###### Bukti proses daemon sudah tidak ada
![dokumentasi stop_daemon](https://github.com/user-attachments/assets/15e6a59a-280a-45c6-95a0-ec67c027529d)
###### Bukti proses daemon telah di kill
![dokumentasi stop_daemon_2](https://github.com/user-attachments/assets/302252c2-5c6b-4dd3-9ff3-86f4f42b6b35)
##### **Fungsi fail_user (./debugmon fail <user>)**
###### 1. Menulis deny file
###### 2. Menyalin dan memodifikasi .bashrc user agar logout otomatis saat login jika diblokir
###### 3. Membunuh semua proses user
```sh
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
```
##### **Fungsi revert_user (./debugmon revert <user>)**
###### 1. Menghapus deny file
###### 2. Menghapus blokir dari .bashrc
```sh
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
```
###### Output:
![dokumentasi revert_user](https://github.com/user-attachments/assets/a2947697-630e-4634-bfad-3f70729749ad)
##### **Fungsi main**
###### Mengatur alur berdasarkan argumen (list, daemon, stop, fail, revert)
```sh
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
```


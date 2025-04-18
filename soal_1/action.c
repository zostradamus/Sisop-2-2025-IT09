#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <curl/curl.h>
#include <zip.h>

int is_single_char(const char *name) {
    return strlen(name) == 5 &&
           ((name[0] >= '0' && name[0] <= '9') || (name[0] >= 'a' && name[0] <= 'z')) &&
           strcmp(name + 1, ".txt") == 0;
}

int is_digit_file(const char *name) {
    return strlen(name) == 5 && isdigit(name[0]);
}

int is_alpha_file(const char *name) {
    return strlen(name) == 5 && isalpha(name[0]);
}

void create_filtered_folder() {
    struct stat st = {0};
    if (stat("Filtered", &st) == -1) {
        mkdir("Filtered", 0700);
    }
}

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

int compare_strings(const void *a, const void *b) {
    const char *str1 = (const char *)a;
    const char *str2 = (const char *)b;
    return strcmp(str1, str2);
}

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

int main(int argc, char *argv[]) {
    download_and_extract();

    if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        if (strcmp(argv[2], "Filter") == 0) {
            filter_files();
        } else if (strcmp(argv[2], "Combine") == 0) {
            combine_files();
        } else if (strcmp(argv[2], "Decode") == 0) {
            decode_file();
        } else {
            printf("Argumen tidak dikenali.\nGunakan:\n  ./action -m Filter\n  ./action -m Combine\n  ./action -m Decode\n");
        }
    } else {
        printf("Cara pakai:\n  ./action -m Filter  (untuk filter file)\n  ./action -m Combine (untuk gabung file)\n  ./action -m Decode  (untuk decode ROT13)\n");
    }

    return 0;
}

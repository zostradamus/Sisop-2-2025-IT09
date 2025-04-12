#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

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
    char dirName[10];
    char fileName[50];
    char destName[100];
    DIR *d;
    struct dirent *dir;

    mkdir("Filtered", 0777); // Buat folder Filtered kalau belum ada

    for (char c = 'A'; c <= 'Z'; c++) {
        snprintf(dirName, sizeof(dirName), "Clue%c", c);
        d = opendir(dirName);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                // Lewati "." dan ".."
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                    continue;

                // Ambil nama tanpa ekstensi
                char name[20];
                strncpy(name, dir->d_name, strlen(dir->d_name) - 4);
                name[strlen(dir->d_name) - 4] = '\0';

                // Cek panjang nama harus 1
                if (strlen(name) == 1 && (isalnum(name[0]))) {
                    snprintf(fileName, sizeof(fileName), "%s/%s", dirName, dir->d_name);
                    snprintf(destName, sizeof(destName), "Filtered/%s", dir->d_name);
                    printf("Memindahkan file valid: %s\n", fileName);
                    rename(fileName, destName); // Pindahkan file ke Filtered
                }
            }
            closedir(d);
        } else {
            printf("Gagal membuka direktori %s\n", dirName);
        }
    }
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

    qsort(digits, digit_count, sizeof(digits[0]), (int(*)(const void*, const void*)) strcmp);
    qsort(alphas, alpha_count, sizeof(alphas[0]), (int(*)(const void*, const void*)) strcmp);

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

int main(int argc, char *argv[]) {
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

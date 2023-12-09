#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_PATH_LEN 256
#define MAX_FILENAME_LEN 128

void processFile(char *inputDir, char *outputDir, char *filename) {
    char inputFile[MAX_PATH_LEN];
    char outputFile[MAX_PATH_LEN];
    sprintf(inputFile, "%s/%s", inputDir, filename);
    sprintf(outputFile, "%s/%s_statistica.txt", outputDir, filename);

    FILE *input = fopen(inputFile, "r");
    FILE *output = fopen(outputFile, "w");

    if (input == NULL || output == NULL) {
        perror("Eroare la deschiderea fișierelor");
        exit(EXIT_FAILURE);
    }

    int lines = 0;
    char buffer[MAX_PATH_LEN];
    while (fgets(buffer, MAX_PATH_LEN, input) != NULL) {
        lines++;
    }

    fprintf(output, "Statisticile pentru %s:\nNumărul de linii: %d\n", filename, lines);

    fclose(input);
    fclose(output);
}

void convertToGrayInMemory(char *inputFile) {
    FILE *file = fopen(inputFile, "rb+");
    if (file == NULL) {
        perror("Eroare la deschiderea fișierului");
        exit(EXIT_FAILURE);
    }

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    int width = header[18] + (header[19] << 8) + (header[20] << 16) + (header[21] << 24);
    int height = header[22] + (header[23] << 8) + (header[24] << 16) + (header[25] << 24);

    int dataOffset = header[10] + (header[11] << 8) + (header[12] << 16) + (header[13] << 24);

    fseek(file, dataOffset, SEEK_SET);

    unsigned char pixel[3];
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            fread(pixel, sizeof(unsigned char), 3, file);

            unsigned char gray = 0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0];

            pixel[0] = pixel[1] = pixel[2] = gray;

            fseek(file, -3, SEEK_CUR);
            fwrite(pixel, sizeof(unsigned char), 3, file);
        }
    }

    fclose(file);
}

void printExitStatus(pid_t pid, int status) {
    if (WIFEXITED(status)) {
        printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", pid, WEXITSTATUS(status));
    } else {
        printf("Procesul cu pid-ul %d s-a încheiat cu un cod de terminare neobișnuit\n", pid);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Utilizare: %s <director_intrare> <director_iesire>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *inputDir = argv[1];
    char *outputDir = argv[2];

    DIR *dir = opendir(inputDir);
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului de intrare");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char inputFile[MAX_PATH_LEN];
            if (strlen(inputDir) + strlen(entry->d_name) + 2 > MAX_PATH_LEN) {
                fprintf(stderr, "Numele fișierului este prea lung: %s/%s\n", inputDir, entry->d_name);
                continue; // sau alte acțiuni pentru gestionarea fișierelor cu nume prea lungi
            } else {
                strncpy(inputFile, inputDir, MAX_PATH_LEN - strlen(entry->d_name) - 2);
                strncat(inputFile, "/", MAX_PATH_LEN - strlen(inputFile) - 1);
                strncat(inputFile, entry->d_name, MAX_PATH_LEN - strlen(inputFile) - 1);
            }

            if (strstr(entry->d_name, ".bmp") != NULL) {
                pid_t pid = fork();

                if (pid == 0) {
                    convertToGrayInMemory(inputFile);
                    exit(EXIT_SUCCESS);
                } else if (pid < 0) {
                    perror("Eroare la fork");
                    exit(EXIT_FAILURE);
                }
            } else {
                pid_t pid = fork();

                if (pid == 0) {
                    processFile(inputDir, outputDir, entry->d_name);
                    exit(EXIT_SUCCESS);
                } else if (pid < 0) {
                    perror("Eroare la fork");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    closedir(dir);

    int status;
    pid_t finished_pid;
    while ((finished_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printExitStatus(finished_pid, status);
    }

    return 0;
}

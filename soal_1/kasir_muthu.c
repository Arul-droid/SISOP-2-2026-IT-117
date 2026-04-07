#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void run_and_wait(char **args) {
    pid_t pid = fork();

    if (pid < 0) {
        printf("[ERROR] Aiyaa! Proses gagal, file atau folder tidak ditemukan.\n");
        exit(1);
    }

    if (pid == 0) {
        // Child process: ganti dengan command yang diminta
        execvp(args[0], args);
        // Jika execvp gagal (file/command tidak ditemukan)
        exit(1);
    } else {
        // Parent process: tunggu child selesai (sequential)
        int status;
        waitpid(pid, &status, 0);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("[ERROR] Aiyaa! Proses gagal, file atau folder tidak ditemukan.\n");
            exit(1);
        }
    }
}

int main() {
    // Step 1: Buat folder brankas_kedai
    char *cmd1[] = {"mkdir", "brankas_kedai", NULL};
    run_and_wait(cmd1);

    // Step 2: Salin buku_hutang.csv ke brankas_kedai/
    char *cmd2[] = {"cp", "buku_hutang.csv", "brankas_kedai/", NULL};
    run_and_wait(cmd2);

    // Step 3: grep "Belum Lunas" → daftar_penunggak.txt
    // Pakai bash -c karena perlu redirect '>' (system() dilarang)
    char *cmd3[] = {
        "bash", "-c",
        "grep \"Belum Lunas\" brankas_kedai/buku_hutang.csv > brankas_kedai/daftar_penunggak.txt",
        NULL
    };
    run_and_wait(cmd3);

    // Step 4: Kompres brankas_kedai → rahasia_muthu.zip
    char *cmd4[] = {"zip", "-r", "rahasia_muthu.zip", "brankas_kedai", NULL};
    run_and_wait(cmd4);

    printf("[INFO] Fuhh, selamat! Buku hutang dan daftar penagihan berhasil diamankan.\n");
    return 0;
}
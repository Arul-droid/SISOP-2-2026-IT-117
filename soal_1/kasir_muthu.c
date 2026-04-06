#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void run_and_wait(char *desc, char **args) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork gagal");
        exit(1);
    }
    if (pid == 0) {
        // child process
        execvp(args[0], args);
        // Kalau execvp gagal
        exit(1);
    } else {
        // parent menunggu child selesai
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("[ERROR] Aiyaa! Proses gagal, file atau folder tidak ditemukan.\n");
            exit(1);
        }
    }
}

int main() {
    // Step 1: mkdir brankas_kedai
    char *cmd1[] = {"mkdir", "brankas_kedai", NULL};
    run_and_wait("mkdir", cmd1);

    // Step 2: cp buku_hutang.csv brankas_kedai/
    char *cmd2[] = {"cp", "buku_hutang.csv", "brankas_kedai/", NULL};
    run_and_wait("cp", cmd2);

    // Step 3: grep "Belum Lunas" dengan redirect menggunakan bash -c
    char *cmd3[] = {
        "bash", "-c",
        "grep \"Belum Lunas\" brankas_kedai/buku_hutang.csv > brankas_kedai/daftar_penunggak.txt",
        NULL
    };
    run_and_wait("grep", cmd3);

    // Step 4: zip -r rahasia_muthu.zip brankas_kedai
    char *cmd4[] = {"zip", "-r", "rahasia_muthu.zip", "brankas_kedai", NULL};
    run_and_wait("zip", cmd4);

    printf("[INFO] Fuhh, selamat! Buku hutang dan daftar penagihan berhasil diamankan.\n");
    return 0;
}

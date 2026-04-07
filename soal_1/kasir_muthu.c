#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void run_and_wait(char **args);

int main() {
    // program 1: merupakan program yang nantinya akan menyimpan string mkdir, brankas_kedai ke cmd1 dengan pointer agar dapat menyimpan banyak string dengan panjang berbeda beda dan yang kemudian akan berjalan dan membuat direktori baru dengan nama brankas_kedai.
    char *cmd1[] = {"mkdir", "brankas_kedai", NULL};
    run_and_wait(cmd1);

    // program 2: merupakan program yang nantinya akan menyimpan string cp, buku_hutang.csv, brankas_kedai/ ke cmd2 dengan pointer agar dapat menyimpan banyak string dengan panjang berbeda beda dan yang kemudian akan berjalan dan menyalin file buku_hutang.csv ke dalam direktori brankas_kedai.
    char *cmd2[] = {"cp", "buku_hutang.csv", "brankas_kedai/", NULL};
    run_and_wait(cmd2);

    // program 3: merupakan program yang nantinya akan menyimpan string grep, Belum Lunas, brankas_kedai/buku_hutang.csv, >, brankas_kedai/daftar_penunggak.txt ke cmd3 dengan pointer agar dapat menyimpan banyak string dengan panjang berbeda beda dan yang kemudian akan berjalan dan mengeksekusi command grep untuk mencari baris yang mengandung "Belum Lunas" di dalam file buku_hutang.csv dan hasilnya akan disimpan ke dalam file daftar_penunggak.txt di dalam direktori brankas_kedai. Karena terdapat simbol '>' yang merupakan operator redirection.
    // Pakai bash -c karena perlu redirect '>' karena penggunaan (system() dilarang)
    char *cmd3[] = {
        "bash", "-c",
        "grep \"Belum Lunas\" brankas_kedai/buku_hutang.csv > brankas_kedai/daftar_penunggak.txt",
        NULL
    };
    run_and_wait(cmd3);

    // program 4: merupakan program yang nantinya akan menyimpan string zip, -r, rahasia_muthu.zip, brankas_kedai ke cmd4 dengan pointer agar dapat menyimpan banyak string dengan panjang berbeda beda dan yang kemudian akan berjalan dan mengeksekusi command zip untuk mengarsipkan direktori brankas_kedai beserta isinya ke dalam file rahasia_muthu.zip. Karena terdapat opsi -r yang berarti recursive untuk mengarsipkan seluruh isi direktori.
    char *cmd4[] = {"zip", "-r", "rahasia_muthu.zip", "brankas_kedai", NULL};
    run_and_wait(cmd4);

    // program 5: merupakan perintah yang akan menampilkan pesan "[INFO] Fuhh, selamat! Buku hutang dan daftar penagihan berhasil diamankan." ke layar setelah semua proses sebelumnya berhasil dijalankan tanpa error. Pesan ini memberikan informasi bahwa semua langkah untuk mengamankan buku hutang dan daftar penagihan telah selesai dengan sukses.
    printf("[INFO] Fuhh, selamat! Buku hutang dan daftar penagihan berhasil diamankan.\n");
    return 0;
}

// merupakan fungsi yang akan menjalankan command yang diberikan dalam bentuk array string (args) dengan menggunakan fork dan execvp. Fungsi ini akan membuat proses anak untuk menjalankan command, sementara proses induk akan menunggu hingga proses anak selesai. Jika terjadi error saat fork atau execvp, fungsi ini akan mencetak pesan error dan keluar dengan status 1. Jika proses anak selesai dengan status tidak normal, fungsi ini juga akan mencetak pesan [ERROR] aiyaa! Proses gagal, file atau folder tidak ditemukan. dan keluar dengan status 1. Fungsi ini memastikan bahwa setiap command yang dijalankan berhasil sebelum melanjutkan ke command berikutnya.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

// merupakan definisi dari beberapa konstanta yang akan digunakan dalam program. LOG_FILE adalah nama file log yang akan digunakan untuk menyimpan pesan log, CONTRACT adalah nama file kontrak yang akan dipantau, dan LINE1 adalah string yang harus ada di baris pertama file kontrak agar dianggap valid.
#define LOG_FILE    "work.log"
#define CONTRACT    "contract.txt"
#define LINE1       "\"A promise to keep going, even when unseen.\"\n"

volatile sig_atomic_t running = 1;

/* ── File Handling (dari modul: fprintf, fopen, fclose) ── */

void write_log(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");   // mode 'a' = append
    if (!f) return;
    fprintf(f, "%s\n", msg);
    fclose(f);
}

void get_timestamp(char *buf, size_t len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm);
}

void write_contract(int restored) {
    FILE *f = fopen(CONTRACT, "w");   // mode 'w' = tulis ulang
    if (!f) return;
    char ts[32];
    get_timestamp(ts, sizeof(ts));
    fprintf(f, LINE1);
    fprintf(f, "\n");
    if (restored)
        fprintf(f, "restored at: %s\n", ts);
    else
        fprintf(f, "created at: %s\n", ts);
    fclose(f);
}

int contract_valid() {
    FILE *f = fopen(CONTRACT, "r");   // mode 'r' = baca
    if (!f) return 0;
    char line[256];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    fclose(f);
    return strcmp(line, LINE1) == 0;
}

/* ── Signal Handler ── */

void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/* ── Daemonize (sesuai langkah modul) ── */

void daemonize() {
    pid_t pid;

    // Langkah 1: Fork dan matikan parent
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);  // parent exit

    // Langkah 2: umask
    umask(0);

    // Langkah 3: Buat SID baru (setsid)
    if (setsid() < 0) exit(EXIT_FAILURE);

    // Fork kedua agar tidak bisa acquire terminal
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // Langkah 4: Ubah working directory
    if ((chdir(".")) < 0) exit(EXIT_FAILURE);

    // Langkah 5: Tutup file descriptor standar
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect ke /dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

int main() {
    daemonize();

    // Tangkap sinyal untuk cleanup
    signal(SIGTERM, handle_signal);
    signal(SIGINT,  handle_signal);

    const char *statuses[] = {"[awake]", "[drifting]", "[numbness]"};
    srand((unsigned)time(NULL));

    // Buat contract.txt saat daemon pertama jalan
    write_contract(0);

    time_t last_log = time(NULL);

    // Langkah 6: Loop utama (dari modul: while + sleep)
    while (running) {
        sleep(1);

        time_t now = time(NULL);

        // Monitor contract.txt
        if (access(CONTRACT, F_OK) != 0) {
            // File dihapus → recreate
            write_contract(1);
        } else if (!contract_valid()) {
            // Isi diubah → log + restore
            write_log("contract violated.");
            write_contract(1);
        }

        // Tulis log setiap 5 detik
        if (now - last_log >= 5) {
            char msg[64];
            snprintf(msg, sizeof(msg), "still working... %s",
                     statuses[rand() % 3]);
            write_log(msg);
            last_log = now;
        }
    }

    // Daemon dihentikan
    write_log("We really weren't meant to be together");
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define LOG_FILE   "work.log"
#define CONTRACT   "contract.txt"
#define CONTRACT_LINE1 "\"A promise to keep going, even when unseen.\"\n"

volatile sig_atomic_t running = 1;

/* ── helpers ── */

void write_log(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
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
    FILE *f = fopen(CONTRACT, "w");
    if (!f) return;
    char ts[32];
    get_timestamp(ts, sizeof(ts));
    fprintf(f, CONTRACT_LINE1);
    fprintf(f, "\n");
    if (restored)
        fprintf(f, "restored at: %s\n", ts);
    else
        fprintf(f, "created at: %s\n", ts);
    fclose(f);
}

/* Baca isi contract.txt, return 1 jika isinya valid (baris pertama sesuai) */
int contract_valid() {
    FILE *f = fopen(CONTRACT, "r");
    if (!f) return 0;
    char line[256];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    fclose(f);
    return strcmp(line, CONTRACT_LINE1) == 0;
}

/* ── signal handler ── */

void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/* ── daemonize ── */

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // parent keluar

    setsid(); // jadi session leader

    // fork kedua supaya tidak bisa acquire terminal
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir(".");

    // Tutup stdin/stdout/stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect ke /dev/null
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_WRONLY); // stderr
}

/* ── main ── */

int main() {
    daemonize();

    signal(SIGTERM, handle_signal);
    signal(SIGINT,  handle_signal);

    const char *statuses[] = {"[awake]", "[drifting]", "[numbness]"};
    srand((unsigned)time(NULL));

    // Buat contract.txt saat pertama kali
    write_contract(0);

    time_t last_log = time(NULL);

    while (running) {
        sleep(1);

        time_t now = time(NULL);

        /* ── Cek contract.txt ── */
        if (access(CONTRACT, F_OK) != 0) {
            // File dihapus → restore
            write_contract(1);
        } else if (!contract_valid()) {
            // Isi diubah → log + restore
            write_log("contract violated.");
            write_contract(1);
        }

        /* ── Tulis log setiap 5 detik ── */
        if (now - last_log >= 5) {
            char msg[64];
            const char *status = statuses[rand() % 3];
            snprintf(msg, sizeof(msg), "still working... %s", status);
            write_log(msg);
            last_log = now;
        }
    }

    // Daemon dihentikan
    write_log("We really weren't meant to be together");
    return 0;
}

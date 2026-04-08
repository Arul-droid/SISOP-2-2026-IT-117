#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

// merupakan definisi dari beberapa konstanta yang akan digunakan dalam program. LOGFILE adalah nama file log yang akan digunakan untuk menyimpan pesan log, CONTRACT adalah nama file kontrak yang akan dipantau, dan LINE1 adalah string yang harus ada di baris pertama file kontrak agar dianggap valid.
#define LOGFILE    "work.log"
#define CONTRACT    "contract.txt"
#define LINECONTRACT  "\"A promise to keep going, even when unseen.\"\n"

// Penanda global agar program tetap running atau berhenti
volatile sig_atomic_t running = 1;


/* ── File Handling (dari modul: fprintf, fopen, fclose) ── */
// Prosedur untuk mengukir pesan ke dalam file log tanpa menghapus isi sebelumnya
void writeLog(const char *msg) {
    FILE *f = fopen(LOGFILE, "a");   // mode 'a' = append
    if (!f) return;
    fprintf(f, "%s\n", msg);
    fclose(f);
}

// Mengambil detak waktu saat ini untuk dicatat dalam sistem
void getTimestamp(char *buf, size_t len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm);
}

// Prosedur untuk menulis ulang file kontrak jika hilang atau rusak
void writeContract(int restored) {
    FILE *f = fopen(CONTRACT, "w");   // mode 'w' = tulis ulang
    if (!f) return;
    char ts[32];
    getTimestamp(ts, sizeof(ts));
    fprintf(f, LINECONTRACT);
    fprintf(f, "\n");
    if (restored)
        fprintf(f, "restored at: %s\n", ts);
    else
        fprintf(f, "created at: %s\n", ts);
    fclose(f);
}

// Fungsi pengecekan: Apakah isi kontrak masih murni sesuai dengan LINE1?
int contractValid() {
    FILE *f = fopen(CONTRACT, "r");   // mode 'r' = baca
    if (!f) return 0;
    char line[256];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    fclose(f);
    return strcmp(line, LINECONTRACT) == 0;
}

// Signal Handler

void handleSignal(int signal) {
    (void)signal;
    running = 0;
}

// Daemonize 

void daemonize() {
    pid_t pid;

    // Tahap 1: Melahirkan proses anak dan mematikan induknya
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);  // parent exit

    // Tahap 2: Reset izin akses file
    umask(0);

    // Tahap 3: Memisahkan diri dari sesi terminal
    if (setsid() < 0) exit(EXIT_FAILURE);

    // Tahap tambahan: Fork sekali lagi agar benar-benar terisolasi
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // Tahap 4: Menetap di direktori saat ini
    if ((chdir(".")) < 0) exit(EXIT_FAILURE);

    // Tahap 5: Menutup jalur komunikasi standar (input, output, error)
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Mengalihkan sampah data ke /dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

int main() {
    // Inisiasi proses daemon
    daemonize();

    // Tangkap sinyal untuk cleanup
    signal(SIGTERM, handleSignal);
    signal(SIGINT,  handleSignal);

    // Array kata-kata status untuk variasi log
    const char *statuses[] = {"[awake]", "[drifting]", "[numbness]"};
    srand((unsigned)time(NULL));

    // Buat contract.txt saat daemon pertama jalan
    writeContract(0);

    time_t last_log = time(NULL);

    // Langkah 6: Loop utama (dari modul: while + sleep)
    while (running) {
        sleep(1); // Beristirahat sejenak setiap detik

        time_t now = time(NULL);

        // Mengawasi integritas file CONTRACT
        if (access(CONTRACT, F_OK) != 0) {
            // Jika file lenyap, buat kembali
            writeContract(1);
        } else if (!contractValid()) {
            // Jika isi berubah (tidak valid), catat di log lalu perbaiki
            writeLog("contract violated.");
            writeContract(1);
        }

        // Tulis log setiap 5 detik
        if (now - last_log >= 5) {
            char msg[64];
            snprintf(msg, sizeof(msg), "still working... %s",
                     statuses[rand() % 3]);
            writeLog(msg);
            last_log = now;
        }
    }

    // Daemon dihentikan
    writeLog("We really weren't meant to be together");
    return 0;
}
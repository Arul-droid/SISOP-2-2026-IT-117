#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define LOVE_FILE   "LoveLetter.txt"
#define LOG_FILE    "ethereal.log"
#define PID_FILE    "angel.pid"

/* ── 4 kalimat cinta Dika ── */
const char *letters[] = {
    "aku akan fokus pada diriku sendiri",
    "aku mencintaimu dari sekarang hingga selamanya",
    "aku akan menjauh darimu, hingga takdir mempertemukan kita di versi kita yang terbaik.",
    "kalau aku dilahirkan kembali, aku tetap akan terus menyayangimu"
};
#define N_LETTERS 4

/* ══════════════════════════════════════════
   LOGGING
══════════════════════════════════════════ */
void write_log(const char *process, const char *status) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    fprintf(f, "[%02d:%02d:%04d]-[%02d:%02d:%02d]_%s_%s\n",
            tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            process, status);
    fclose(f);
}

/* ══════════════════════════════════════════
   BASE64
══════════════════════════════════════════ */
static const char b64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64_encode(const unsigned char *in, size_t len, char *out) {
    size_t i, o = 0;
    for (i = 0; i < len; i += 3) {
        unsigned char a = in[i];
        unsigned char b = (i+1 < len) ? in[i+1] : 0;
        unsigned char c = (i+2 < len) ? in[i+2] : 0;
        out[o++] = b64[a >> 2];
        out[o++] = b64[((a & 3) << 4) | (b >> 4)];
        out[o++] = (i+1 < len) ? b64[((b & 0xF) << 2) | (c >> 6)] : '=';
        out[o++] = (i+2 < len) ? b64[c & 0x3F] : '=';
    }
    out[o] = '\0';
}

/* decode satu karakter base64 */
static int b64val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

/* Decode base64 string → plain text, return panjang output */
size_t base64_decode(const char *in, size_t len, unsigned char *out) {
    size_t i, o = 0;
    for (i = 0; i + 3 < len; i += 4) {
        int a = b64val(in[i]);
        int b = b64val(in[i+1]);
        int c = b64val(in[i+2]);
        int d = b64val(in[i+3]);
        if (a < 0 || b < 0) break;
        out[o++] = (a << 2) | (b >> 4);
        if (in[i+2] != '=') out[o++] = ((b & 0xF) << 4) | (c >> 2);
        if (in[i+3] != '=') out[o++] = ((c & 3) << 6) | d;
    }
    return o;
}

/* ══════════════════════════════════════════
   FITUR SECRET  – tulis kalimat acak
══════════════════════════════════════════ */
void do_secret() {
    write_log("secret", "RUNNING");
    srand((unsigned)time(NULL));
    const char *msg = letters[rand() % N_LETTERS];

    FILE *f = fopen(LOVE_FILE, "w");
    if (!f) { write_log("secret", "ERROR"); return; }
    fprintf(f, "%s\n", msg);
    fclose(f);
    write_log("secret", "SUCCESS");
}

/* ══════════════════════════════════════════
   FITUR SURPRISE – enkripsi LoveLetter.txt
══════════════════════════════════════════ */
void do_surprise() {
    write_log("surprise", "RUNNING");

    FILE *f = fopen(LOVE_FILE, "r");
    if (!f) { write_log("surprise", "ERROR"); return; }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    unsigned char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);

    /* encode */
    char *enc = malloc(sz * 2 + 4);
    base64_encode(buf, sz, enc);
    free(buf);

    f = fopen(LOVE_FILE, "w");
    if (!f) { free(enc); write_log("surprise", "ERROR"); return; }
    fprintf(f, "%s\n", enc);
    free(enc);
    fclose(f);

    write_log("surprise", "SUCCESS");
}

/* ══════════════════════════════════════════
   FITUR DECRYPT  – ./angel -decrypt
══════════════════════════════════════════ */
void do_decrypt() {
    write_log("decrypt", "RUNNING");

    FILE *f = fopen(LOVE_FILE, "r");
    if (!f) {
        fprintf(stderr, "[ERROR] LoveLetter.txt tidak ditemukan.\n");
        write_log("decrypt", "ERROR");
        return;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    char *enc = malloc(sz + 1);
    fread(enc, 1, sz, f);
    fclose(f);

    /* strip newline */
    long real_len = sz;
    while (real_len > 0 && (enc[real_len-1] == '\n' || enc[real_len-1] == '\r'))
        enc[--real_len] = '\0';

    unsigned char *dec = malloc(real_len + 4);
    size_t dec_len = base64_decode(enc, real_len, dec);
    free(enc);

    f = fopen(LOVE_FILE, "w");
    if (!f) { free(dec); write_log("decrypt", "ERROR"); return; }
    fwrite(dec, 1, dec_len, f);
    fclose(f);
    free(dec);

    printf("[INFO] LoveLetter.txt berhasil didekripsi.\n");
    write_log("decrypt", "SUCCESS");
}

/* ══════════════════════════════════════════
   DAEMONIZE
══════════════════════════════════════════ */
void daemonize(char *argv0) {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);   /* parent keluar */

    setsid();

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);   /* session leader keluar */

    umask(0);
    chdir(".");

    /* simpan PID */
    FILE *pf = fopen(PID_FILE, "w");
    if (pf) { fprintf(pf, "%d\n", getpid()); fclose(pf); }

    /* ganti nama proses → "maya" */
    memset(argv0, 0, strlen(argv0));
    strcpy(argv0, "maya");

    /* redirect fd */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

/* ══════════════════════════════════════════
   MAIN LOOP DAEMON
══════════════════════════════════════════ */
void run_daemon(char *argv0) {
    daemonize(argv0);

    srand((unsigned)(time(NULL) ^ getpid()));
    time_t last = 0;

    while (1) {
        time_t now = time(NULL);
        if (now - last >= 10) {
            do_secret();
            do_surprise();
            last = now;
        }
        sleep(1);
    }
}

/* ══════════════════════════════════════════
   KILL
══════════════════════════════════════════ */
void do_kill() {
    write_log("kill", "RUNNING");

    FILE *pf = fopen(PID_FILE, "r");
    if (!pf) {
        fprintf(stderr, "[ERROR] Daemon belum berjalan (pid file tidak ditemukan).\n");
        write_log("kill", "ERROR");
        return;
    }
    pid_t pid;
    fscanf(pf, "%d", &pid);
    fclose(pf);

    if (kill(pid, SIGTERM) == 0) {
        remove(PID_FILE);
        printf("[INFO] Daemon (PID %d) berhasil dihentikan.\n", pid);
        write_log("kill", "SUCCESS");
    } else {
        fprintf(stderr, "[ERROR] Gagal menghentikan daemon.\n");
        write_log("kill", "ERROR");
    }
}

/* ══════════════════════════════════════════
   MAIN
══════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Penggunaan:\n");
        printf("  ./angel -daemon  : jalankan sebagai daemon (nama proses: maya)\n");
        printf("  ./angel -decrypt : decrypt LoveLetter.txt\n");
        printf("  ./angel -kill    : kill proses\n");
        return 0;
    }

    if (strcmp(argv[1], "-daemon") == 0) {
        run_daemon(argv[0]);
    } else if (strcmp(argv[1], "-decrypt") == 0) {
        do_decrypt();
    } else if (strcmp(argv[1], "-kill") == 0) {
        do_kill();
    } else {
        printf("Argumen tidak dikenal: %s\n", argv[1]);
        printf("Gunakan: -daemon | -decrypt | -kill\n");
    }

    return 0;
}

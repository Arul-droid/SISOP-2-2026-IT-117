#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

// --- KONFIGURASI NAMA FILE ---
// Di sini kita tentukan nama-nama file yang bakal dipakai.
#define LOVE_FILE  "LoveLetter.txt" // File utama tempat nyimpen pesan rahasia.
#define LOG_FILE   "ethereal.log"   // Buku harian program, semua aktivitas dicatat di sini.
#define PID_FILE   "angel.pid"      // Kertas catatan buat nyimpen ID proses (biar gampang dicari pas mau dimatiin).

// List pesan yang bakal diputar secara acak. Bisa kamu ganti sesuka hati.
const char *letters[] = {
    "aku akan fokus pada diriku sendiri",
    "aku mencintaimu dari sekarang hingga selamanya",
    "aku akan menjauh darimu, hingga takdir mempertemukan kita di versi kita yang terbaik.",
    "kalau aku dilahirkan kembali, aku tetap akan terus menyayangimu"
};
#define N_LETTERS 4

/* --- CATATAN AKTIVITAS --- */
// Fungsi ini tugasnya nulis 'curhatan' program ke file log.
// Ada timestamp (waktu) biar kita tahu kapan kejadiannya.
void write_log(const char *process, const char *status) {
    FILE *f = fopen(LOG_FILE, "a"); // Buka file log, mode 'a' biar tulisan baru nggak numpuk tulisan lama.
    if (!f) return;
    
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    // Nulis ke log dengan format: [Tanggal-Jam]_Proses_Status
    fprintf(f, "[%02d:%02d:%04d]-[%02d:%02d:%02d]_%s_%s\n",
            tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            process, status);
    fclose(f);
}

/* --- LOGIKA PENGACAKAN HURUF (BASE64) --- */
// Tabel ini standarnya Base64, isinya kumpulan karakter buat nyamarin teks asli.
static const char b64t[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Fungsi buat 'bungkus' teks asli jadi kode acak Base64.
void base64_encode(const unsigned char *in, size_t len, char *out) {
    size_t i, o = 0;
    for (i = 0; i < len; i += 3) {
        unsigned char a = in[i];
        unsigned char b = (i+1 < len) ? in[i+1] : 0;
        unsigned char c = (i+2 < len) ? in[i+2] : 0;
        
        // Di sini proses matematikanya, geser-geser bit biar teksnya berubah.
        out[o++] = b64t[a >> 2];
        out[o++] = b64t[((a & 3) << 4) | (b >> 4)];
        out[o++] = (i+1 < len) ? b64t[((b & 0xF) << 2) | (c >> 6)] : '=';
        out[o++] = (i+2 < len) ? b64t[c & 0x3F] : '=';
    }
    out[o] = '\0'; // Tandain akhir dari tulisan.
}

// Kebalikannya, ini buat 'buka bungkus' kode Base64 balik ke teks asli.
size_t base64_decode(const char *in, size_t len, unsigned char *out) {
    // Fungsi pembantu buat nyocokin karakter ke nilai angka.
    auto int b64val = [](char c) {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62; if (c == '/') return 63;
        return -1;
    };

    size_t i, o = 0;
    for (i = 0; i + 3 < len; i += 4) {
        int a = b64val(in[i]),   b = b64val(in[i+1]);
        int c = b64val(in[i+2]), d = b64val(in[i+3]);
        if (a < 0 || b < 0) break;
        
        out[o++] = (a << 2) | (b >> 4);
        if (in[i+2] != '=') out[o++] = ((b & 0xF) << 4) | (c >> 2);
        if (in[i+3] != '=') out[o++] = ((c & 3)   << 6) | d;
    }
    return o;
}

/* --- TUGAS-TUGAS PROGRAM --- */

// Pilih satu pesan acak, terus tulis ke file.
void do_secret() {
    write_log("secret", "RUNNING");
    srand((unsigned)time(NULL));

    FILE *f = fopen(LOVE_FILE, "w"); // Mode 'w' bakal hapus isi lama dan nulis yang baru.
    if (!f) { write_log("secret", "ERROR"); return; }
    fprintf(f, "%s\n", letters[rand() % N_LETTERS]);
    fclose(f);
    write_log("secret", "SUCCESS");
}

// Baca pesan tadi, terus 'disandikan' pakai Base64.
void do_surprise() {
    write_log("surprise", "RUNNING");
    FILE *f = fopen(LOVE_FILE, "r");
    if (!f) { write_log("surprise", "ERROR"); return; }

    // Cari tahu berapa panjang tulisan di file.
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);

    unsigned char *buf = (unsigned char*)malloc(sz + 1);
    fread(buf, 1, sz, f); buf[sz] = '\0';
    fclose(f);

    // Proses nyamarin teks.
    char *enc = (char*)malloc(sz * 2 + 4);
    base64_encode(buf, sz, enc);
    free(buf);

    // Simpan hasil samaran tadi ke file yang sama.
    f = fopen(LOVE_FILE, "w");
    if (!f) { free(enc); write_log("surprise", "ERROR"); return; }
    fprintf(f, "%s\n", enc);
    free(enc); fclose(f);
    write_log("surprise", "SUCCESS");
}

// Buat kita yang mau baca lagi, fungsi ini bakal balikin teks samaran ke teks asli.
void do_decrypt() {
    write_log("decrypt", "RUNNING");
    FILE *f = fopen(LOVE_FILE, "r");
    if (!f) { printf("Filenya nggak ada!\n"); return; }

    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *enc = (char*)malloc(sz + 1);
    fread(enc, 1, sz, f); fclose(f);

    // Bersihin karakter aneh kayak Enter di akhir teks.
    long rlen = sz;
    while (rlen > 0 && (enc[rlen-1] == '\n' || enc[rlen-1] == '\r')) enc[--rlen] = '\0';

    unsigned char *dec = (unsigned char*)malloc(rlen + 4);
    size_t dec_len = base64_decode(enc, rlen, dec);
    free(enc);

    // Tulis balik hasil aslinya ke file.
    f = fopen(LOVE_FILE, "w");
    fwrite(dec, 1, dec_len, f);
    fclose(f); free(dec);
    printf("Selesai! LoveLetter.txt udah bisa dibaca lagi.\n");
    write_log("decrypt", "SUCCESS");
}

/* --- RITUAL JADI DAEMON --- */
// Di sini program 'melepaskan diri' dari terminal biar jalan diem-diem di background.
void daemonize(char *argv0) {
    pid_t pid = fork(); // Belah diri jadi dua.
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // Parent (proses asli) mati, tinggalin anaknya.

    if (setsid() < 0) exit(EXIT_FAILURE); // Bikin sesi baru biar nggak nempel ke terminal.

    pid = fork(); // Belah diri lagi biar bener-bener lepas.
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // Catat ID prosesnya biar kita tahu siapa yang lagi jalan di belakang.
    FILE *pf = fopen(PID_FILE, "w");
    if (pf) { fprintf(pf, "%d\n", getpid()); fclose(pf); }

    // Ganti nama proses pas dicek di Task Manager/Top jadi "maya".
    memset(argv0, 0, strlen(argv0));
    strcpy(argv0, "maya");

    umask(0); chdir("."); // Atur izin file dan pindah folder kerja.
    // Tutup jalur input/output ke terminal karena kita udah nggak butuh itu lagi.
    close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
}

/* --- JANTUNG PROGRAM --- */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Cara pakai:\n ./angel -daemon\n ./angel -decrypt\n ./angel -kill\n");
        return 0;
    }

    // Cek perintah yang kamu ketik di terminal.
    if (strcmp(argv[1], "-daemon") == 0) {
        daemonize(argv[0]);
        time_t last = 0;
        while (1) {
            time_t now = time(NULL);
            // Setiap 10 detik, bikin pesan baru terus langsung di-encode.
            if (now - last >= 10) {
                do_secret();
                do_surprise();
                last = now;
            }
            sleep(1); // Istirahat bentar biar nggak makan CPU.
        }
    } 
    else if (strcmp(argv[1], "-decrypt") == 0) do_decrypt();
    else if (strcmp(argv[1], "-kill") == 0) {
        // Logika sederhana buat matiin proses berdasarkan file PID.
        FILE *pf = fopen(PID_FILE, "r");
        if (pf) {
            int pid; fscanf(pf, "%d", &pid); fclose(pf);
            kill(pid, SIGTERM); remove(PID_FILE);
            printf("Daemon udah dimatiin.\n");
        }
    }
    
    return 0;
}
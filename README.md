# SISOP-2-2026-IT-117

<div align="center">

# Laporan Resmi Praktikum Sistem Operasi
## Modul 2 — Process & Daemon

</div>

---

# Daftar Isi

- [Soal 1 - Kasbon Warga Kampung Durian Runtuh](#soal-1---kasbon-warga-kampung-durian-runtuh)
- [Soal 2 - The world never stops, even when you feel tired](#soal-2---the-world-never-stops-even-when-you-feel-tired)
- [Soal 3 - Angel](#soal-3---angel)

---

## Soal 1 - Kasbon Warga Kampung Durian Runtuh

*Author: NINN*

### Penjelasan

Program `kasir_muthu.c` dibuat untuk membantu Uncle Muthu mengamankan data buku hutang secara otomatis dan berurutan. Program ini mengandalkan **Sequential Process** menggunakan kombinasi `fork()`, fungsi `exec()`, dan `waitpid()`. Penggunaan `system()` dilarang.

#### Alur Sequential Process

Setiap langkah dijalankan oleh child process yang dibuat menggunakan `fork()`. Parent process wajib menunggu tiap child selesai menggunakan `waitpid()` sebelum melanjutkan ke langkah berikutnya. Jika salah satu child gagal (exit code != 0), program langsung berhenti dan mencetak pesan error.

```c
void run_and_wait(char **args) {
    pid_t pid = fork();
    // fork() membuat salinan proses saat ini (child process)
    // pid == 0  → kita berada di dalam child process
    // pid >  0  → kita berada di dalam parent process, nilai pid = PID child
    // pid <  0  → fork() gagal

    if (pid == 0) {
        // Bagian child: ganti image proses dengan command yang diminta
        // execvp() mencari program secara otomatis di PATH sistem
        // args[0] = nama program, args[1..n] = argumen, diakhiri NULL
        execvp(args[0], args);

        // Baris ini hanya tercapai kalau execvp gagal (misal command tidak ditemukan)
        exit(1);
    } else {
        int status;
        // waitpid() membuat parent menunggu child dengan PID tertentu selesai
        // Inilah yang membuat prosesnya "sequential" — parent tidak lanjut
        // ke step berikutnya sebelum child selesai
        waitpid(pid, &status, 0);

        // WIFEXITED(status): bernilai true jika child berhenti secara normal
        // WEXITSTATUS(status): mengambil exit code dari child
        // Jika exit code bukan 0, berarti command gagal dieksekusi
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("[ERROR] Aiyaa! Proses gagal, file atau folder tidak ditemukan.\n");
            exit(1); // hentikan seluruh program, tidak lanjut ke step berikutnya
        }
    }
}
```

#### Langkah-Langkah Program

**Step 1 — Membuat folder `brankas_kedai`**

Menggunakan `mkdir -p` agar tidak error jika folder sudah ada.

```c
// Flag "-p" membuat mkdir tidak error walau folder sudah ada
// Tanpa "-p", jika folder sudah ada maka mkdir return exit code 1
// dan run_and_wait akan membaca itu sebagai kegagalan lalu menghentikan program
char *cmd1[] = {"mkdir", "-p", "brankas_kedai", NULL};
run_and_wait(cmd1); // parent menunggu folder benar-benar terbuat sebelum lanjut
```

**Step 2 — Menyalin `buku_hutang.csv` ke dalam `brankas_kedai`**

```c
// cp menyalin file sumber ke direktori tujuan
// Jika buku_hutang.csv tidak ada di current directory,
// cp return exit code != 0 dan program berhenti dengan pesan error
char *cmd2[] = {"cp", "buku_hutang.csv", "brankas_kedai/", NULL};
run_and_wait(cmd2); // parent menunggu proses copy selesai sebelum lanjut
```

**Step 3 — Mencari data "Belum Lunas" dan menyimpannya ke `daftar_penunggak.txt`**

Karena `system()` dilarang, redirect `>` dijalankan lewat `bash -c` di dalam `execvp`.

```c
// execvp tidak bisa langsung menggunakan redirect '>' karena '>' adalah fitur shell,
// bukan fitur dari program grep itu sendiri
// Solusinya: jalankan bash sebagai perantara dengan flag -c
// bash -c "string" → bash akan mengeksekusi string tersebut sebagai perintah shell
// sehingga karakter '>' diinterpretasi sebagai redirect output oleh bash
char *cmd3[] = {
    "bash", "-c",
    "grep \"Belum Lunas\" brankas_kedai/buku_hutang.csv > brankas_kedai/daftar_penunggak.txt",
    NULL
};
run_and_wait(cmd3); // parent menunggu grep dan redirect selesai sebelum lanjut
```

**Step 4 — Mengompresi `brankas_kedai` menjadi `rahasia_muthu.zip`**

```c
// zip -r = recursive, mengompresi seluruh isi folder beserta subfolder di dalamnya
// Argumen pertama setelah flag = nama file zip yang akan dibuat
// Argumen kedua = folder yang akan dikompres
char *cmd4[] = {"zip", "-r", "rahasia_muthu.zip", "brankas_kedai", NULL};
run_and_wait(cmd4); // parent menunggu proses zip selesai
```

Jika semua langkah berhasil, program mencetak:

```
[INFO] Fuhh, selamat! Buku hutang dan daftar penagihan berhasil diamankan.
```

### Output

**Kompilasi dan menjalankan program**

![assets/soal1/kasir_muthu_berhasil_jalan.png](assets/soal1/kasir_muthu_berhasil_jalan.png)

**Isi folder `brankas_kedai` (hasil `tree`)**

![alt text](assets/soal1/isi_brankas_kedai.png)

**Isi `daftar_penunggak.txt`**

![alt text](assets/soal1/isi_daftar_penunggak.png)

**File `rahasia_muthu.zip` berhasil dibuat**

![alt text](assets/soal1/zip_dibuat.png)

**Error handling**

![alt text](assets/soal1/error_handling(noFile).png)
![alt text](assets/soal1/error_handling(dirAda).png)

### Kendala

`zip` perlu diinstall terlebih dahulu di WSL dengan `sudo apt install zip -y`.

---

## Soal 2 - The world never stops, even when you feel tired

*Author: MINT*

### Penjelasan

Program `contract_daemon.c` dibuat sebagai daemon yang berjalan di background secara terus-menerus. Program mengikuti langkah pembuatan daemon sesuai modul: **fork → umask → setsid → chdir → close fd standar → loop utama**.

#### Proses Daemonize

```c
void daemonize() {
    pid_t pid = fork();
    // Fork pertama: buat child lalu matikan parent
    // Tujuannya agar shell mengira program sudah selesai
    // dan proses bisa berjalan bebas di background
    if (pid < 0) exit(EXIT_FAILURE); // fork gagal
    if (pid > 0) exit(EXIT_SUCCESS); // parent keluar, child tetap jalan

    // umask(0): atur permission mask menjadi 0
    // Agar file yang dibuat daemon punya permission sesuai yang kita set,
    // tidak terpengaruh permission default dari sistem
    umask(0);

    // setsid(): membuat session baru dan menjadi session leader
    // Memutus hubungan proses dari controlling terminal
    // Daemon tidak ikut mati ketika terminal pengontrolnya ditutup
    if (setsid() < 0) exit(EXIT_FAILURE);

    // Fork kedua: agar proses bukan lagi session leader
    // Session leader masih berpotensi membuka terminal baru
    // Dengan fork kedua, child tidak bisa membuka terminal → benar-benar daemon
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // session leader keluar

    // chdir("."): tetap di direktori saat ini
    // Bisa juga chdir("/") untuk pindah ke root agar lebih aman
    chdir(".");

    // Tutup tiga file descriptor standar agar daemon tidak berinteraksi dengan terminal
    // STDIN_FILENO  = 0 (input)
    // STDOUT_FILENO = 1 (output)
    // STDERR_FILENO = 2 (error output)
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Buka /dev/null untuk menempati fd 0, 1, 2 yang baru saja ditutup
    // Mencegah fd tersebut dipakai tidak sengaja oleh file lain
    open("/dev/null", O_RDONLY); // fd 0 → /dev/null (baca dari null = selalu kosong)
    open("/dev/null", O_WRONLY); // fd 1 → /dev/null (tulis ke null = dibuang)
    open("/dev/null", O_WRONLY); // fd 2 → /dev/null (error juga dibuang)
}
```

#### Fitur-Fitur Daemon

**Menulis log setiap 5 detik**

Daemon menulis `still working...` beserta status acak (`[awake]`, `[drifting]`, `[numbness]`) ke file `work.log` setiap 5 detik.

```c
// time(NULL) mengembalikan Unix timestamp (detik sejak 1 Jan 1970)
// Selisih dua timestamp dipakai sebagai timer sederhana
// tanpa perlu thread terpisah atau alarm
if (now - last_log >= 5) {
    char msg[64];
    // rand() % 3 menghasilkan angka acak 0, 1, atau 2
    // dipakai sebagai index ke array statuses = {"[awake]","[drifting]","[numbness]"}
    snprintf(msg, sizeof(msg), "still working... %s", statuses[rand() % 3]);
    write_log(msg);
    last_log = now; // reset timer ke waktu sekarang
}
```

**Membuat `contract.txt` saat daemon pertama kali jalan**

```c
void write_contract(int restored) {
    // fopen mode "w": buat file baru atau timpa jika sudah ada
    FILE *f = fopen(CONTRACT, "w");
    if (!f) return;

    char ts[32];
    get_timestamp(ts, sizeof(ts)); // ambil waktu sekarang dalam format string

    // Baris pertama selalu sama — ini yang dipakai untuk validasi isi file
    fprintf(f, "\"A promise to keep going, even when unseen.\"\n\n");

    // Baris kedua berbeda tergantung kondisi:
    // restored = 0 → file baru dibuat pertama kali → "created at"
    // restored = 1 → file dibuat ulang karena dihapus/diubah → "restored at"
    if (restored)
        fprintf(f, "restored at: %s\n", ts);
    else
        fprintf(f, "created at: %s\n", ts);

    fclose(f); // wajib dipanggil untuk flush buffer ke disk
}
```

**Monitor dan restore `contract.txt` jika dihapus**

Daemon mengecek keberadaan file setiap 1 detik menggunakan `access()`. Jika tidak ada, file dibuat ulang dalam waktu ~1 detik dengan baris kedua berubah menjadi `restored at: <timestamp>`.

```c
// access(path, F_OK): cek keberadaan file tanpa membukanya
// Mengembalikan 0 jika file ada, -1 jika tidak ada
// Dicek setiap iterasi loop yang berjalan tiap 1 detik
if (access(CONTRACT, F_OK) != 0) {
    // File tidak ada → buat ulang dengan mode "restored"
    write_contract(1); // argumen 1 = restored mode
}
```

**Monitor dan restore `contract.txt` jika isinya diubah**

Baris pertama file dibandingkan dengan konten asli. Jika berbeda, daemon menulis `contract violated.` ke log lalu merestore file.

```c
// contract_valid() membuka file, membaca baris pertama dengan fgets(),
// lalu membandingkannya dengan string asli menggunakan strcmp()
// strcmp() return 0 jika string identik, non-zero jika berbeda
} else if (!contract_valid()) {
    write_log("contract violated."); // catat pelanggaran ke work.log
    write_contract(1);               // restore isi file ke kondisi semula
}
```

**Pesan saat daemon dihentikan**

Ketika daemon menerima sinyal `SIGTERM` (misalnya dari `pkill`), daemon menulis pesan terakhir ke log:

```
We really weren't meant to be together
```

```c
// volatile sig_atomic_t: tipe data yang dijamin aman diakses dari signal handler
// volatile mencegah compiler mengoptimasi variabel ini (harus selalu dibaca dari memory)
volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    (void)sig;    // cast ke void untuk suppress compiler warning "unused parameter"
    running = 0;  // ubah flag → loop utama akan berhenti di iterasi berikutnya
}

// Daftarkan handler: ketika sinyal diterima, OS akan memanggil fungsi handle_signal
signal(SIGTERM, handle_signal); // SIGTERM dikirim oleh pkill / kill <pid>
signal(SIGINT,  handle_signal); // SIGINT dikirim oleh Ctrl+C

// Loop utama berjalan selama flag running bernilai 1
while (running) {
    sleep(1);
    // ... cek contract.txt, tulis log setiap 5 detik ...
}
// Setelah running = 0, loop selesai, lanjut ke baris berikutnya
write_log("We really weren't meant to be together");
```

### Output

**Daemon berjalan di background**

![alt text](assets/soal2/daemon_berhasil_jalan.png)

**Isi `work.log` setelah beberapa detik**

![alt text](assets/soal2/isi_workLog.png)

**`contract.txt` dibuat saat daemon start**

![alt text](assets/soal2/isi_contract.png)

**`contract.txt` direstore**

![alt text](assets/soal2/isi_contract_diubah.png)

**Log "contract violated." setelah isi diubah**

![alt text](assets/soal2/mengubah_contract.png)
![alt text](assets/soal2/isi_workLog_diubah.png)

**Pesan terakhir di log setelah daemon dihentikan**

![alt text](assets/soal2/mematikan_daemon.png)
![alt text](assets/soal2/isi_workLog_dimatikan.png)


---

## Soal 3 - Angel

*Author: MINT*

### Penjelasan

Program `angel.c` adalah daemon dengan nama proses `"maya"` yang memiliki tiga mode operasi: `-daemon`, `-decrypt`, dan `-kill`. Program menggunakan proses daemonize dua kali fork sesuai modul, dilengkapi fitur enkripsi Base64 dan sistem logging.

#### Struktur Argumen Program

```
Penggunaan:
  ./angel -daemon  : jalankan sebagai daemon (nama proses: maya)
  ./angel -decrypt : decrypt LoveLetter.txt
  ./angel -kill    : kill proses
```

#### Daemonize dan Rename Proses

Setelah double fork dan `setsid()`, nama proses diganti menjadi `"maya"` dengan menimpa `argv[0]`.

```c
// argv[0] adalah pointer ke string nama program yang tersimpan di memory proses
// memset mengisi seluruh byte string lama dengan null (\0) untuk membersihkannya
// strcpy kemudian menulis "maya" di posisi memory yang sama
// Efeknya: ps aux akan menampilkan "maya" sebagai nama proses, bukan "./angel"
int pjg_nama = strlen(nama_proses);
memset(nama_proses, 0, pjg_nama); // hapus nama lama karakter per karakter
strcpy(nama_proses, "maya");      // tulis nama baru di posisi yang sama
```

PID daemon disimpan ke `angel.pid` agar bisa di-kill nantinya.

```c
// Simpan PID proses saat ini ke file teks
// Tujuannya agar instance lain dari program (./angel -kill) bisa membaca
// PID ini dan mengirimkan sinyal ke daemon yang sedang berjalan
FILE *pf = fopen(PIDKU, "w");
if (pf) {
    fprintf(pf, "%d\n", getpid()); // getpid() mengembalikan PID proses saat ini
    fclose(pf);
}
```

#### Fitur Secret — Menulis Kalimat Acak

Setiap 10 detik, daemon memilih satu dari empat kalimat secara acak dan menuliskannya ke `LoveLetter.txt`.

```c
void tulis_surat() {
    // srand dengan seed dari time ^ getpid agar urutan random berbeda tiap run
    // Tanpa srand atau seed yang tetap, rand() selalu menghasilkan urutan sama
    srand((unsigned)time(NULL));

    int pilihan = rand() % 4; // hasilkan angka acak 0-3 sebagai index array

    // fopen mode "w": buat file baru / timpa isi lama jika file sudah ada
    FILE *f = fopen(SURAT, "w");
    if (f == NULL) { /* error handling */ return; }

    fprintf(f, "%s\n", isi_surat[pilihan]); // tulis kalimat yang dipilih
    fclose(f);
}
```

#### Fitur Surprise — Enkripsi Base64

Setelah fitur secret menulis kalimat, fitur surprise langsung mengenkripsi isi `LoveLetter.txt` menggunakan Base64 dengan implementasi manual.

```c
void sembunyikan_surat() {
    // Baca seluruh isi file ke buffer
    FILE *f = fopen(SURAT, "r");
    fseek(f, 0, SEEK_END);   // pindahkan cursor ke akhir file
    long ukuran = ftell(f);  // posisi cursor saat ini = ukuran file dalam byte
    rewind(f);               // kembalikan cursor ke posisi awal

    unsigned char *isi = malloc(ukuran + 1); // +1 untuk null terminator '\0'
    fread(isi, 1, ukuran, f); // baca sebanyak ukuran byte dari file ke buffer isi
    isi[ukuran] = '\0';
    fclose(f);

    // Base64 mengubah setiap 3 byte input menjadi 4 karakter output
    // Sehingga ukuran output ≈ ukuran_input * 4/3
    // ukuran * 2 + 8 lebih dari cukup sebagai buffer yang aman
    char *hasil = malloc(ukuran * 2 + 8);
    enkode(isi, ukuran, hasil); // proses enkripsi Base64

    // Timpa LoveLetter.txt dengan hasil enkripsi
    f = fopen(SURAT, "w");
    fprintf(f, "%s\n", hasil);
    fclose(f);

    free(isi);   // bebaskan memory yang sudah tidak dipakai
    free(hasil); // wajib free() untuk mencegah memory leak
}
```

**Cara kerja enkripsi Base64:**

```c
// Base64 mengubah setiap 3 byte (24 bit) menjadi 4 karakter ASCII (6 bit per karakter)
// Karakter yang digunakan: A-Z (0-25), a-z (26-51), 0-9 (52-61), + (62), / (63)
void enkode(const unsigned char *masuk, size_t panjang, char *keluar) {
    for (i = 0; i < panjang; i += 3) {
        unsigned char x = masuk[i];                           // byte pertama
        unsigned char y = (i+1 < panjang) ? masuk[i+1] : 0; // byte kedua (0 jika tidak ada)
        unsigned char z = (i+2 < panjang) ? masuk[i+2] : 0; // byte ketiga (0 jika tidak ada)

        // Ambil 6 bit paling kiri dari byte x
        keluar[pos++] = tabel_b64[x >> 2];
        // Ambil 2 bit kanan x digabung dengan 4 bit kiri y
        keluar[pos++] = tabel_b64[((x & 3) << 4) | (y >> 4)];
        // Ambil 4 bit kanan y digabung dengan 2 bit kiri z
        // Jika byte kedua tidak ada → tulis '=' sebagai padding
        keluar[pos++] = (i+1 < panjang) ? tabel_b64[((y & 0xF) << 2) | (z >> 6)] : '=';
        // Ambil 6 bit paling kanan dari z
        // Jika byte ketiga tidak ada → tulis '=' sebagai padding
        keluar[pos++] = (i+2 < panjang) ? tabel_b64[z & 0x3F] : '=';
    }
    keluar[pos] = '\0'; // null terminator agar bisa dipakai sebagai string C
}
```

#### Fitur Decrypt — `./angel -decrypt`

Membaca isi `LoveLetter.txt` yang terenkripsi, melakukan Base64 decode, dan menuliskannya kembali ke file dalam bentuk teks asli.

```c
void buka_surat() {
    FILE *f = fopen(SURAT, "r");
    if (!f) {
        // Tulis ke stderr (bukan stdout) agar pesan error tidak tercampur output normal
        // stderr tidak di-buffer, pesan langsung tampil di terminal
        fprintf(stderr, "[ERROR] LoveLetter.txt tidak ditemukan.\n");
        return;
    }

    // Baca seluruh isi file terenkripsi ke buffer
    fseek(f, 0, SEEK_END);
    long ukuran = ftell(f);
    rewind(f);
    char *terenkripsi = malloc(ukuran + 1);
    fread(terenkripsi, 1, ukuran, f);
    fclose(f);

    // Buang karakter newline (\n) dan carriage return (\r) di akhir string
    // karena karakter ini bukan bagian dari Base64 dan akan menyebabkan decode gagal
    long pjg = ukuran;
    while (pjg > 0 && (terenkripsi[pjg-1] == '\n' || terenkripsi[pjg-1] == '\r'))
        terenkripsi[--pjg] = '\0';

    // Decode: balik proses enkripsi, 4 karakter Base64 → 3 byte data asli
    unsigned char *asli = malloc(pjg + 4);
    size_t pjg_asli = dekode(terenkripsi, pjg, asli);

    // Tulis hasil decode kembali ke file
    // fwrite dipakai (bukan fprintf) karena data bisa mengandung byte non-printable
    f = fopen(SURAT, "w");
    fwrite(asli, 1, pjg_asli, f); // tulis pjg_asli byte dari buffer asli ke file
    fclose(f);

    free(terenkripsi);
    free(asli);
    printf("[INFO] LoveLetter.txt berhasil didekripsi.\n");
}
```

#### Fitur Kill — `./angel -kill`

Membaca PID dari `angel.pid` dan mengirimkan `SIGTERM` ke daemon.

```c
void matiin_daemon() {
    // Buka file PID yang disimpan saat daemon pertama kali dijalankan
    FILE *pf = fopen(PIDKU, "r");
    if (!pf) {
        // File tidak ada → daemon belum pernah dijalankan atau sudah berhenti
        fprintf(stderr, "[ERROR] Daemon belum berjalan.\n");
        return;
    }

    pid_t target_pid;
    fscanf(pf, "%d", &target_pid); // baca angka PID dari file
    fclose(pf);

    // kill(pid, signal): sistem call untuk mengirim sinyal ke proses lain
    // SIGTERM (nomor 15): meminta proses berhenti secara graceful
    // Berbeda dengan SIGKILL (9) yang langsung membunuh tanpa bisa ditangkap
    // Mengembalikan 0 jika berhasil, -1 jika gagal (misal PID tidak ada)
    if (kill(target_pid, SIGTERM) == 0) {
        remove(PIDKU); // hapus file PID karena daemon sudah tidak berjalan
        printf("[INFO] Daemon (PID %d) berhasil dihentikan.\n", target_pid);
    } else {
        // kill gagal: mungkin daemon sudah mati tapi file PID belum terhapus
        fprintf(stderr, "[ERROR] Gagal menghentikan daemon.\n");
    }
}
```

#### Sistem Logging — `ethereal.log`

Setiap aktivitas dicatat ke `ethereal.log` dengan format:

```
[dd:mm:yyyy]-[hh:mm:ss]_nama-proses_STATUS
```

```c
void simpan_log(const char *nama, const char *status) {
    // fopen mode "a" (append): tulis di akhir file tanpa menghapus isi sebelumnya
    // Berbeda dengan mode "w" yang menimpa file dari awal setiap kali dibuka
    FILE *fp = fopen(CATATAN, "a");
    if (fp == NULL) return;

    time_t skrg = time(NULL);            // Unix timestamp: detik sejak 1 Jan 1970
    struct tm *waktu = localtime(&skrg); // konversi timestamp ke struct tm berisi
                                         // hari, bulan, tahun, jam, menit, detik

    // %02d: cetak integer, lebar minimal 2 digit, dipadding angka 0 jika perlu
    // tm_mon dimulai dari 0 (Januari = 0, Desember = 11), jadi perlu +1
    // tm_year dihitung dari tahun 1900, jadi perlu +1900 untuk tahun sebenarnya
    fprintf(fp, "[%02d:%02d:%04d]-[%02d:%02d:%02d]_%s_%s\n",
        waktu->tm_mday,       // hari (1-31)
        waktu->tm_mon + 1,    // bulan (1-12)
        waktu->tm_year + 1900,// tahun (misal 2026)
        waktu->tm_hour,       // jam (0-23)
        waktu->tm_min,        // menit (0-59)
        waktu->tm_sec,        // detik (0-59)
        nama, status);

    fclose(fp);
}
```

Contoh output log:
```
[07:04:2026]-[12:00:00]_secret_RUNNING
[07:04:2026]-[12:00:00]_secret_SUCCESS
[07:04:2026]-[12:00:05]_surprise_RUNNING
[07:04:2026]-[12:00:05]_surprise_SUCCESS
```

Status yang digunakan: `RUNNING`, `SUCCESS`, `ERROR`.

### Output

![alt text](assets/soal3/penggunaan.png)

**Daemon berjalan dan nama proses menjadi "maya"**

![alt text](assets/soal3/maya_berjalan.png)

**Isi `LoveLetter.txt` dalam bentuk terenkripsi Base64**

![alt text](assets/soal3/isi_loveletter.png)

**Hasil decrypt `LoveLetter.txt`**

![alt text](assets/soal3/isi_loveletter_dekripsi.png)

**Isi `ethereal.log`**

![alt text](assets/soal3/isi_ethereal.png)

**Menghentikan daemon dengan `./angel -kill`**

![alt text](assets/soal3/matikan_daemon.png)

**Error handling — file tidak ditemukan**

![alt text](assets/soal3/error_handling(fileTidakAda).png)

**Error handling — daemon belum berjalan**

![alt text](assets/soal3/error_handling(daemonBelumJalan).png)

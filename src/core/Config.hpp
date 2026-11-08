#pragma once
// Pengaturan runtime TeamGdMod. Disimpan ke database (lihat Db.hpp) setiap kali ada perubahan,
// jadi tidak hilang walau game ditutup paksa atau pengguna keluar dari level.

namespace tgm {
    struct Config {
        // ================= Hack =================
        bool  noclip         = false; // tidak mati saat menabrak
        bool  noclipP1       = true;  // Noclip berlaku untuk pemain 1
        bool  noclipP2       = true;  // Noclip berlaku untuk pemain 2
        int   batasTabrakan  = 0;     // mati bila tabrakan melebihi angka ini (0 = tanpa batas)
        int   akurasiMin     = 0;     // mati bila akurasi Noclip di bawah persen ini (0 = nonaktif)
        bool  autoSafeMode   = true;  // otomatis Mode Aman (progres tidak tersimpan) bila ada cheat aktif
        bool  noDeathEffect  = false; // lewati animasi ledakan saat mati
        bool  respawnCepat   = false; // respawn lebih cepat setelah mati
        int   respawnMs      = 100;   // jeda respawn cepat dalam milidetik (0 - 1000)
        bool  autoCheckpoint = false; // pasang checkpoint otomatis di mode practice
        int   checkpointDtk  = 5;     // jeda checkpoint otomatis (1 - 30 detik)
        float speed          = 1.f;   // pengali kecepatan game (0.1 - 3.0)
        bool  lompatBebas    = false; // bisa lompat di udara (eksperimental)
        bool  tanpaGuncang   = false; // matikan guncangan layar
        bool  tanpaCermin    = false; // abaikan portal cermin
        bool  bukaSemua      = false; // buka semua ikon dan warna (hanya di perangkat ini)
        bool  jedaSaatBuka   = true;  // jeda level otomatis saat menu dibuka di dalam level

        // ================= HUD: isi =================
        bool  showHud        = true;  // tampilkan HUD
        bool  showFps        = true;  // FPS
        bool  showCps        = false; // klik per detik
        bool  showClicks     = true;  // jumlah klik per percobaan
        bool  showClock      = false; // jam
        bool  showPercent    = false; // persentase level
        bool  showAttempt    = false; // nomor percobaan
        bool  showFrame      = false; // frame fisika saat ini
        bool  showTime       = false; // waktu level berjalan
        bool  showDeaths     = false; // jumlah mati selama sesi
        bool  showBest       = false; // persentase terbaik level ini
        bool  showLevelName  = false; // nama level
        bool  showAkurasi    = false; // akurasi Noclip

        // ================= HUD: gaya =================
        int   hudOpacity     = 70;    // kepekatan dalam persen (10 - 100)
        int   hudScale       = 100;   // ukuran dalam persen (50 - 150)
        int   hudCorner      = 0;     // 0 kiri atas, 1 kanan atas, 2 kiri bawah, 3 kanan bawah, 4 tengah atas, 5 tengah bawah
        int   hudColor       = 0;     // indeks warna (0 - 7)
        int   hudFont        = 0;     // 0 chat, 1 besar, 2 emas
        bool  hudPanel       = true;  // latar gelap di belakang teks HUD
        int   hudOffX        = 0;     // geser horizontal (-100 .. 100)
        int   hudOffY        = 0;     // geser vertikal (-100 .. 100)

        // ================= Replay Bot =================
        int   slot           = 1;     // slot macro (1 - 5)
        bool  autoSaveMacro  = false; // simpan rekaman otomatis saat level selesai
        bool  autoPlay       = false; // putar macro otomatis saat level mulai dari awal
        bool  autoMuat       = false; // muat macro slot terpilih otomatis saat level dibuka

        // ================= Tombol logo =================
        bool  logoTampil     = true;  // tampilkan tombol logo mengambang
        bool  logoDiLevel    = true;  // tetap tampil saat bermain di dalam level
        bool  logoMagnet     = true;  // menempel ke tepi layar setelah dilepas
        bool  logoRedup      = true;  // meredup otomatis bila tidak disentuh
        int   logoUkuran     = 100;   // ukuran dalam persen (50 - 200)
        int   logoOpasitas   = 90;    // kepekatan dalam persen (20 - 100)
        float logoX          = 0.94f; // posisi horizontal (0 - 1 dari lebar layar)
        float logoY          = 0.62f; // posisi vertikal (0 - 1 dari tinggi layar)

        // ================= Musik Practice =================
        bool  musikPracticeAsli = true; // musik di mode Practice sama dengan mode normal (sinkron dengan level)

        // ================= Input Viewer =================
        bool  ivTampil       = false; // tampilkan tombol input di layar saat bermain
        int   ivPosisi       = 3;     // 0 kiri atas, 1 kanan atas, 2 kiri bawah, 3 kanan bawah, 4 tengah atas, 5 tengah bawah
        int   ivUkuran       = 100;   // ukuran dalam persen (50 - 200)
        int   ivOpasitas     = 80;    // kepekatan dalam persen (20 - 100)
        int   ivWarna        = 1;     // indeks warna tombol yang ditekan (0 - 7)
        int   ivOffX         = 0;     // geser horizontal (-150 .. 150)
        int   ivOffY         = 0;     // geser vertikal (-150 .. 150)

        // ================= Presisi Klik =================
        int   cbfMode        = 0;     // 0 = bawaan GD, 1 = Klik di Langkah, 2 = Click Between Frames (eksperimental)
        bool  turbo          = false; // klik otomatis selama tombol lompat ditahan
        int   turboCps       = 10;    // kecepatan klik otomatis (5 - 30 per detik)

        // ================= Tambahan =================
        bool  konfirmasiKeluar = false; // minta konfirmasi sebelum keluar level dari layar jeda
        bool  tombolRestart  = false; // tombol restart kecil di layar saat bermain
        bool  showMatiTerakhir = false; // HUD: persen tempat mati terakhir
        bool  hitbox         = false; // tampilkan hitbox objek dan pemain
        bool  hitboxMati     = false; // tampilkan hitbox saat mati
        bool  frameStepper   = false; // tombol Jeda / Langkah frame di layar

        // ================= Fitur v1.6 =================
        bool  modeRingan     = false; // Mode Ringan: LDM otomatis, tanpa efek mati / guncangan, HUD lebih hemat, animasi menu mati
        bool  fpsBypass      = false; // pakai FPS target sendiri
        int   fpsTarget      = 120;   // FPS target (30 - 360)
        bool  safeMode       = false; // Mode Aman manual: progres level tidak pernah tersimpan
        bool  autoPractice   = false; // otomatis masuk mode Practice saat level dibuka
        bool  autoLdm        = false; // nyalakan Low Detail Mode otomatis saat level dibuka
        bool  bekukanPercobaan = false; // nomor percobaan tidak bertambah
        bool  jejak          = false; // gambar jejak lintasan pemain
        int   jejakPanjang   = 200;   // jumlah titik jejak (50 - 600)
        int   jejakWarna     = 1;     // indeks warna jejak (0 - 7)
        bool  persenAkurat   = false; // persentase HUD dengan 2 desimal
        bool  showCheat      = false; // HUD: penanda saat cheat aktif

        // ================= Fitur v1.7 =================
        bool  instantMirror  = false; // portal cermin langsung berpindah tanpa animasi
        bool  autoKill       = false; // otomatis mati pada persen tertentu (untuk latihan bagian level)
        int   autoKillPersen = 50;    // persen tempat Auto Kill (1 - 100)
        bool  muteMati       = false; // musik level dijeda saat pemain mati
        bool  showTestmode   = false; // HUD: penanda Testmode
        bool  showBestRun    = false; // HUD: run terbaik (persen awal - persen akhir)
        bool  showNoclipMati = false; // HUD: berapa kali pemain seharusnya mati (Noclip)
        bool  kebalPaku      = false; // CHEAT: tidak mati karena paku / hazard
        bool  toastCheat     = true;  // peringatan daftar cheat aktif saat masuk level
        int   cheatPos       = 3;     // posisi titik Cheat Indicator (sama seperti hudCorner)
        bool  showLevelId    = false; // HUD: ID level
        bool  showRataMati   = false; // HUD: rata-rata persen tempat mati
        bool  autoJeda       = false; // jeda level otomatis saat game keluar ke background (pindah aplikasi)
        bool  konfirmasiRestart = false; // minta konfirmasi sebelum restart dari layar jeda

        // ================= Antarmuka =================
        int   temaMenu       = 0;     // tema warna menu (0 - 7)
        int   skalaUi        = 100;   // ukuran menu dalam persen (50 - 150)
        int   opasitasMenu   = 92;    // kepekatan latar menu dalam persen (30 - 100)
        int   mmTutup        = 0;     // bitmask jendela menu yang dilipat
    };

    // Akses tunggal ke konfigurasi
    Config& cfg();

    // Muat / simpan dari database
    void loadConfig();
    void saveConfig();    // menulis SEMUA pengaturan (satu kali tulis ke disk)
    void resetConfig();
}

#pragma once
// Pengaturan runtime TeamGdMod. Disimpan otomatis lewat Mod::setSavedValue.

namespace tgm {
    struct Config {
        // --- Hack ---
        bool  noclip         = false; // tidak mati saat menabrak
        bool  noclipP1       = true;  // Noclip berlaku untuk pemain 1
        bool  noclipP2       = true;  // Noclip berlaku untuk pemain 2
        int   batasTabrakan  = 0;     // mati bila tabrakan melebihi angka ini (0 = tanpa batas)
        bool  autoSafeMode   = true;  // otomatis Mode Aman (progres tidak tersimpan) bila ada cheat aktif
        bool  noDeathEffect  = false; // lewati animasi ledakan saat mati
        bool  autoCheckpoint = false; // pasang checkpoint otomatis di mode practice
        int   checkpointDtk  = 5;     // jeda checkpoint otomatis (1 - 30 detik)
        float speed          = 1.f;   // pengali kecepatan game (0.1 - 3.0)

        // --- HUD: isi ---
        bool  showHud        = true;  // tampilkan HUD
        bool  showFps        = true;  // FPS
        bool  showClicks     = true;  // jumlah klik per percobaan
        bool  showClock      = false; // jam
        bool  showPercent    = false; // persentase level
        bool  showAttempt    = false; // nomor percobaan
        bool  showFrame      = false; // frame fisika saat ini
        bool  showTime       = false; // waktu level berjalan
        bool  showDeaths     = false; // jumlah mati selama sesi

        // --- HUD: gaya ---
        int   hudOpacity     = 70;    // kepekatan dalam persen (10 - 100)
        int   hudScale       = 100;   // ukuran dalam persen (50 - 150)
        int   hudCorner      = 0;     // 0 kiri atas, 1 kanan atas, 2 kiri bawah, 3 kanan bawah
        int   hudColor       = 0;     // indeks warna (0 - 4)

        // --- Replay Bot ---
        int   slot           = 1;     // slot macro (1 - 5)
        bool  autoSaveMacro  = false; // simpan rekaman otomatis saat level selesai
        bool  autoPlay       = false; // putar macro otomatis saat level mulai dari awal
    };

    // Akses tunggal ke konfigurasi
    Config& cfg();

    // Muat / simpan dari penyimpanan mod
    void loadConfig();
    void saveConfig();
    void resetConfig();
}

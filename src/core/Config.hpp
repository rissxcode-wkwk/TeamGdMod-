#pragma once
// Pengaturan runtime TeamGdMod. Disimpan otomatis lewat Mod::setSavedValue.

namespace tgm {
    struct Config {
        bool  noclip       = false; // tidak mati saat menabrak
        bool  autoSafeMode = true;  // otomatis Mode Aman (progres tidak tersimpan) bila ada cheat aktif
        bool  showHud      = true;  // tampilkan HUD status
        float speed        = 1.f;   // pengali kecepatan game (0.1 - 3.0)
        int   slot         = 1;     // slot macro bot (1 - 5)
    };

    // Akses tunggal ke konfigurasi
    Config& cfg();

    // Muat / simpan dari penyimpanan mod
    void loadConfig();
    void saveConfig();
}

#pragma once
// Replay Bot TeamGdMod: merekam input per frame fisika lalu memutarnya kembali.
// Modul ini murni data + file (tanpa tipe GD) supaya mudah dirawat dan diuji.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace tgm::bot {
    enum class Mode { Siaga, Rekam, Putar };

    // Satu aksi input: tombol ditekan / dilepas pada frame tertentu
    struct Aksi {
        uint32_t frame   = 0;
        uint8_t  tombol  = 1;     // 1 = lompat, 2 = kiri, 3 = kanan (platformer)
        bool     tekan   = false; // true = ditekan, false = dilepas
        bool     pemain1 = true;
    };

    // --- Status ---
    Mode        mode();
    std::size_t jumlahAksi();
    std::size_t posisi();         // indeks aksi berikutnya saat memutar
    bool        sedangMemutar();  // true selama masih ada aksi yang belum diputar
    uint32_t    frameTerakhir();  // frame aksi terakhir (0 bila kosong)
    double      durasiDetik();    // perkiraan durasi macro (240 frame fisika per detik)

    // --- Kontrol ---
    void ikatLevel(int idLevel);  // dipanggil tiap level dimuat; macro level lain dibuang
    void mulaiRekam();
    void mulaiPutar();
    void berhenti();

    // --- Dipanggil dari hook game ---
    void catat(uint32_t frame, int tombol, bool tekan, bool pemain1);
    void potongDari(uint32_t frame);   // buang aksi dengan frame >= frame (respawn saat merekam)
    void lompatKe(uint32_t frame);     // posisikan pemutaran ke frame (respawn saat memutar)
    std::optional<Aksi> ambil(uint32_t frame); // aksi yang sudah waktunya dijalankan

    // --- File macro (slot 1..5 per level) ---
    bool simpan(int idLevel, int slot, std::string& pesan);
    bool muat(int idLevel, int slot, std::string& pesan);
    bool hapus(int idLevel, int slot, std::string& pesan);
    bool adaFile(int idLevel, int slot);   // apakah slot sudah berisi macro
}

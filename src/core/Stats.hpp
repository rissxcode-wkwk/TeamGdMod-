#pragma once
// Statistik runtime (tidak disimpan ke disk; yang permanen ada di Riwayat.hpp).

#include <array>
#include <cstdint>
#include <ctime>
#include <deque>

namespace tgm {
    // true selama TeamGdMod sendiri yang memicu restart (jangan tampilkan konfirmasi restart)
    inline bool g_lewatRestart = false;

    // Per percobaan / per level: direset tiap level dimuat atau respawn
    struct Stats {
        uint32_t klik            = 0;    // klik pada percobaan saat ini
        uint32_t tabrakan        = 0;    // tabrakan yang diabaikan Noclip pada percobaan saat ini
        uint32_t frameTabrak     = 0;    // jumlah frame yang berisi tabrakan (untuk akurasi)
        uint32_t frameTabrakAkhir = 0xFFFFFFFF; // frame tabrakan terakhir yang dihitung
        float    fps             = 60.f; // FPS terhaluskan
        float    detikCp         = 0.f;  // penghitung waktu checkpoint otomatis
        float    waktuNyata      = 0.f;  // jam internal waktu nyata (detik sejak level dibuka)
        std::deque<float> waktuKlik;     // waktu nyata tiap klik (untuk menghitung CPS)
        float    detikBelumDicatat = 0.f; // waktu main yang belum dimasukkan ke database
        float    persenMatiTerakhir = -1.f; // persen tempat mati terakhir (-1 = belum ada)
        uint32_t matiNoclip      = 0;    // berapa kali pemain seharusnya mati (rentetan tabrakan dihitung satu)
        uint32_t frameMatiNoclipAkhir = 0xFFFFFFFF; // frame tabrakan terakhir untuk menghitung rentetan
        void*    objekMatiNoclipAkhir = nullptr; // objek (paku / sisi balok) yang terakhir ditabrak saat Noclip
        float    runMulai        = 0.f;  // persen tempat percobaan ini dimulai
        float    bestRunA        = 0.f;  // run terbaik di level ini: dari persen ...
        float    bestRunB        = 0.f;  // ... sampai persen ini
        uint32_t jumlahMati      = 0;    // jumlah kematian sesi level ini (untuk rata-rata)
        float    totalPersenMati = 0.f;  // jumlah persen tempat mati (untuk rata-rata)

        // Persentase akurasi Noclip pada percobaan ini (100 = tidak ada tabrakan)
        float akurasi(uint32_t frameSekarang) const {
            if (frameSekarang == 0) return 100.f;
            float a = 100.f - 100.f * static_cast<float>(frameTabrak) / static_cast<float>(frameSekarang);
            return a < 0.f ? 0.f : a;
        }

        // Klik dalam 1 detik terakhir
        int cps() const {
            int n = 0;
            for (auto it = waktuKlik.rbegin(); it != waktuKlik.rend(); ++it) {
                if (waktuNyata - *it <= 1.f) ++n; else break;
            }
            return n;
        }
    };

    inline Stats& stats() {
        static Stats s_stats;
        return s_stats;
    }

    // Per sesi game: bertahan selama game terbuka
    struct Sesi {
        uint32_t mati      = 0;
        uint32_t klik      = 0;
        std::time_t mulai  = std::time(nullptr);
    };

    inline Sesi& sesi() {
        static Sesi s_sesi;
        return s_sesi;
    }
}

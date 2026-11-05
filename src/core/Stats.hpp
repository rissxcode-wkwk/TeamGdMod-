#pragma once
// Statistik runtime (tidak disimpan ke disk).

#include <cstdint>
#include <ctime>

namespace tgm {
    // Per percobaan / per level: direset tiap level dimuat atau respawn
    struct Stats {
        uint32_t klik      = 0;    // klik pada percobaan saat ini
        uint32_t tabrakan  = 0;    // tabrakan yang diabaikan Noclip pada percobaan saat ini
        float    fps       = 60.f; // FPS terhaluskan
        float    detikCp   = 0.f;  // penghitung waktu checkpoint otomatis
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

#pragma once
// Statistik permanen (disimpan di database): total keseluruhan dan per level.
// Penghitung ditulis lazy (ditandai kotor) lalu di-flush berkala dan saat keluar level / game.

#include <cstdint>
#include <string>

namespace tgm::riwayat {
    struct Total {
        int64_t mati       = 0;
        int64_t klik       = 0;
        int64_t detik      = 0;   // total waktu bermain di dalam level
        int64_t percobaan  = 0;
        int64_t selesai    = 0;   // level yang diselesaikan
        int64_t sesi       = 0;   // berapa kali game dibuka
    };

    struct PerLevel {
        bool        ada       = false;
        int64_t     mati      = 0;
        int64_t     klik      = 0;
        int64_t     detik     = 0;
        int64_t     percobaan = 0;
        int64_t     selesai   = 0;
        int         terbaik   = 0;      // persen terbaik yang tercatat TeamGdMod
        std::string nama;
    };

    Total    total();
    PerLevel level(int idLevel);

    void catatSesiBaru();
    void catatMati(int idLevel);
    void catatKlik(int idLevel);
    void catatPercobaan(int idLevel);
    void catatDetik(int idLevel, int64_t detik);
    void catatSelesai(int idLevel);
    void catatTerbaik(int idLevel, int persen);
    void catatNama(int idLevel, std::string const& nama);

    // Sebaran tempat mati: 20 kotak, masing-masing 5% level
    void catatZonaMati(int idLevel, int persen);
    struct ZonaMati { int kotak = -1; int64_t jumlah = 0; };
    ZonaMati zonaTerbanyak(int idLevel);

    int  jumlahLevelTercatat();
    void hapusSemua();
}

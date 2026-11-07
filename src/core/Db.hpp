#pragma once
// Database lokal TeamGdMod.
//
// Semua pengaturan, statistik, dan status fitur disimpan di sini, BUKAN di saved value Geode.
// Alasannya: saved value Geode hanya ditulis ke disk saat game ditutup dengan benar, sedangkan di HP
// game sering dimatikan paksa (keluar dari app, geser dari recent apps, baterai habis). Database ini
// ditulis langsung setiap ada perubahan, jadi data tidak hilang walau game langsung tertutup.
//
// Jaminan:
//  - Tulis atomik: data ditulis ke file sementara lalu di-rename, jadi file tidak pernah setengah tertulis.
//  - Cadangan otomatis (.bak) dari versi sebelumnya, dipakai bila file utama rusak.
//  - Checksum: file rusak / terpotong terdeteksi dan tidak dipakai.

#include <cstddef>
#include <cstdint>
#include <string>

namespace tgm::db {
    // Dipanggil sekali saat data mod selesai dimuat
    void muat();

    // --- Baca ---
    bool        ada(std::string const& kunci);
    std::string ambil(std::string const& kunci, std::string const& bawaan = "");
    int64_t     ambilInt(std::string const& kunci, int64_t bawaan = 0);
    double      ambilDbl(std::string const& kunci, double bawaan = 0.0);
    bool        ambilBool(std::string const& kunci, bool bawaan = false);

    // --- Tulis ---
    // segera = true  -> langsung ditulis ke disk (kecuali sedang dalam tahan()/lepas())
    // segera = false -> hanya ditandai kotor; ditulis oleh flushJikaKotor() / flush()
    void set(std::string const& kunci, std::string const& nilai, bool segera = true);
    void setInt(std::string const& kunci, int64_t nilai, bool segera = true);
    void setDbl(std::string const& kunci, double nilai, bool segera = true);
    void setBool(std::string const& kunci, bool nilai, bool segera = true);
    void tambahInt(std::string const& kunci, int64_t delta);   // penghitung, ditulis lazy

    void hapus(std::string const& kunci);
    void hapusAwalan(std::string const& awalan);

    // Kelompokkan banyak perubahan menjadi satu kali tulis ke disk
    void tahan();
    void lepas();

    // --- Penyimpanan ---
    void flush();            // paksa tulis sekarang (bila ada perubahan)
    void flushJikaKotor();   // murah bila tidak ada perubahan
    bool cadangkan();        // salin database ke file cadangan manual (.manual)
    bool pulihkanDariCadangan();

    // --- Info ---
    std::size_t jumlahKunci();
    std::string lokasi();
    std::string status();    // ringkasan kondisi untuk ditampilkan di menu
}

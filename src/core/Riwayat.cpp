#include "Riwayat.hpp"
#include "Db.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <set>

using namespace geode::prelude;

namespace tgm::riwayat {
    namespace {
        std::string kl(int id, char const* nama) {
            return fmt::format("rw.lv.{}.{}", id, nama);
        }

        // Daftarkan ID level ke daftar ringkas "rw.daftar" (dipakai untuk menghitung level tercatat)
        void daftarkan(int id) {
            auto d = db::ambil("rw.daftar");
            auto token = "," + std::to_string(id) + ",";
            if ((("," + d + ",")).find(token) != std::string::npos) return;
            db::set("rw.daftar", d.empty() ? std::to_string(id) : d + "," + std::to_string(id), false);
        }
    }

    Total total() {
        Total t;
        t.mati      = db::ambilInt("rw.total.mati");
        t.klik      = db::ambilInt("rw.total.klik");
        t.detik     = db::ambilInt("rw.total.detik");
        t.percobaan = db::ambilInt("rw.total.percobaan");
        t.selesai   = db::ambilInt("rw.total.selesai");
        t.sesi      = db::ambilInt("rw.total.sesi");
        return t;
    }

    PerLevel level(int idLevel) {
        PerLevel p;
        p.ada       = db::ada(kl(idLevel, "percobaan")) || db::ada(kl(idLevel, "klik")) || db::ada(kl(idLevel, "mati"));
        p.mati      = db::ambilInt(kl(idLevel, "mati"));
        p.klik      = db::ambilInt(kl(idLevel, "klik"));
        p.detik     = db::ambilInt(kl(idLevel, "detik"));
        p.percobaan = db::ambilInt(kl(idLevel, "percobaan"));
        p.selesai   = db::ambilInt(kl(idLevel, "selesai"));
        p.terbaik   = static_cast<int>(db::ambilInt(kl(idLevel, "terbaik")));
        p.nama      = db::ambil(kl(idLevel, "nama"));
        return p;
    }

    void catatSesiBaru() { db::tambahInt("rw.total.sesi", 1); db::flush(); }

    void catatMati(int id) {
        db::tambahInt("rw.total.mati", 1);
        db::tambahInt(kl(id, "mati"), 1);
    }

    void catatKlik(int id) {
        db::tambahInt("rw.total.klik", 1);
        db::tambahInt(kl(id, "klik"), 1);
    }

    void catatPercobaan(int id) {
        daftarkan(id);
        db::tambahInt("rw.total.percobaan", 1);
        db::tambahInt(kl(id, "percobaan"), 1);
    }

    void catatDetik(int id, int64_t detik) {
        db::tambahInt("rw.total.detik", detik);
        db::tambahInt(kl(id, "detik"), detik);
    }

    void catatSelesai(int id) {
        db::tambahInt("rw.total.selesai", 1);
        db::tambahInt(kl(id, "selesai"), 1);
    }

    void catatTerbaik(int id, int persen) {
        if (persen > db::ambilInt(kl(id, "terbaik"))) {
            db::setInt(kl(id, "terbaik"), persen, false);
        }
    }

    void catatNama(int id, std::string const& nama) {
        if (!nama.empty()) db::set(kl(id, "nama"), nama, false);
    }

    void catatZonaMati(int id, int persen) {
        int kotak = std::clamp(persen / 5, 0, 19);
        db::tambahInt(kl(id, ("z" + std::to_string(kotak)).c_str()), 1);
    }

    ZonaMati zonaTerbanyak(int id) {
        ZonaMati hasil;
        for (int k = 0; k < 20; ++k) {
            auto n = db::ambilInt(kl(id, ("z" + std::to_string(k)).c_str()));
            if (n > hasil.jumlah) { hasil.jumlah = n; hasil.kotak = k; }
        }
        return hasil;
    }

    int jumlahLevelTercatat() {
        // Dihitung dari kunci "rw.lv.<id>.percobaan" yang ada
        int n = 0;
        std::set<int> dilihat;
        // Tidak ada API iterasi di Db, jadi gunakan pencarian ID yang pernah dicatat lewat daftar ringkas
        auto daftar = db::ambil("rw.daftar");
        std::size_t i = 0;
        while (i < daftar.size()) {
            auto j = daftar.find(',', i);
            if (j == std::string::npos) j = daftar.size();
            if (j > i) {
                try { dilihat.insert(std::stoi(daftar.substr(i, j - i))); } catch (...) {}
            }
            i = j + 1;
        }
        n = static_cast<int>(dilihat.size());
        return n;
    }

    void hapusSemua() {
        db::hapusAwalan("rw.");
    }
}

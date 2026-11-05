#include "Bot.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <utility>
#include <vector>

using namespace geode::prelude;

namespace tgm::bot {
    namespace {
        Mode               g_mode = Mode::Siaga;
        std::vector<Aksi>  g_aksi;
        std::size_t        g_indeks = 0;
        std::vector<Aksi>  g_pulihkan;   // tombol yang harus ditekan ulang setelah respawn
        int                g_idLevel = -1;

        // Format file: "TGM1" | u32 jumlah | jumlah x { u32 frame, u8 tombol, u8 bendera }
        // bendera: bit0 = tekan, bit1 = pemain 1. Semua angka little-endian.
        constexpr char          KODE_FILE[4] = {'T', 'G', 'M', '1'};
        constexpr std::size_t   UKURAN_ENTRI = 6;

        std::filesystem::path folderMacro() {
            return Mod::get()->getSaveDir() / "macros";
        }

        std::filesystem::path pathMacro(int idLevel, int slot) {
            return folderMacro() / fmt::format("level{}_slot{}.tgm", idLevel, slot);
        }

        void tulisU32(std::ofstream& o, uint32_t v) {
            char b[4] = {
                static_cast<char>(v & 0xFF), static_cast<char>((v >> 8) & 0xFF),
                static_cast<char>((v >> 16) & 0xFF), static_cast<char>((v >> 24) & 0xFF)
            };
            o.write(b, 4);
        }

        bool bacaU32(std::ifstream& i, uint32_t& v) {
            unsigned char b[4];
            if (!i.read(reinterpret_cast<char*>(b), 4)) return false;
            v = static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8)
              | (static_cast<uint32_t>(b[2]) << 16) | (static_cast<uint32_t>(b[3]) << 24);
            return true;
        }
    }

    Mode        mode()        { return g_mode; }
    std::size_t jumlahAksi()  { return g_aksi.size(); }
    std::size_t posisi()      { return g_indeks; }

    bool sedangMemutar() {
        return g_mode == Mode::Putar && (g_indeks < g_aksi.size() || !g_pulihkan.empty());
    }

    uint32_t frameTerakhir() {
        return g_aksi.empty() ? 0 : g_aksi.back().frame;
    }

    double durasiDetik() {
        return static_cast<double>(frameTerakhir()) / 240.0;
    }

    void ikatLevel(int idLevel) {
        // Level berbeda -> macro lama tidak relevan lagi
        if (idLevel != g_idLevel) {
            g_idLevel = idLevel;
            g_aksi.clear();
        }
        g_indeks = 0;
        g_pulihkan.clear();
        g_mode = Mode::Siaga;
    }

    void mulaiRekam() {
        g_aksi.clear();
        g_indeks = 0;
        g_pulihkan.clear();
        g_mode = Mode::Rekam;
    }

    void mulaiPutar() {
        if (g_aksi.empty()) return;
        g_indeks = 0;
        g_pulihkan.clear();
        g_mode = Mode::Putar;
    }

    void berhenti() {
        g_mode = Mode::Siaga;
        g_pulihkan.clear();
    }

    void catat(uint32_t frame, int tombol, bool tekan, bool pemain1) {
        Aksi a;
        a.frame = frame;
        a.tombol = static_cast<uint8_t>(tombol);
        a.tekan = tekan;
        a.pemain1 = pemain1;
        g_aksi.push_back(a);
    }

    void potongDari(uint32_t frame) {
        std::erase_if(g_aksi, [frame](Aksi const& a) { return a.frame >= frame; });
    }

    void lompatKe(uint32_t frame) {
        g_pulihkan.clear();

        // Aksi terurut menaik berdasarkan frame -> cari aksi pertama dengan frame >= frame
        auto it = std::lower_bound(
            g_aksi.begin(), g_aksi.end(), frame,
            [](Aksi const& a, uint32_t f) { return a.frame < f; }
        );
        g_indeks = static_cast<std::size_t>(it - g_aksi.begin());

        // Hitung tombol mana yang sedang ditahan pada titik ini, lalu tekan ulang
        std::map<std::pair<int, bool>, bool> ditahan;
        for (std::size_t i = 0; i < g_indeks; ++i) {
            ditahan[{g_aksi[i].tombol, g_aksi[i].pemain1}] = g_aksi[i].tekan;
        }
        for (auto const& [kunci, tekan] : ditahan) {
            if (!tekan) continue;
            Aksi a;
            a.frame = frame;
            a.tombol = static_cast<uint8_t>(kunci.first);
            a.tekan = true;
            a.pemain1 = kunci.second;
            g_pulihkan.push_back(a);
        }
    }

    std::optional<Aksi> ambil(uint32_t frame) {
        if (!g_pulihkan.empty()) {
            auto a = g_pulihkan.back();
            g_pulihkan.pop_back();
            return a;
        }
        if (g_indeks < g_aksi.size() && g_aksi[g_indeks].frame <= frame) {
            return g_aksi[g_indeks++];
        }
        return std::nullopt;
    }

    bool simpan(int idLevel, int slot, std::string& pesan) {
        if (g_aksi.empty()) {
            pesan = "Belum ada rekaman untuk disimpan";
            return false;
        }

        std::error_code ec;
        std::filesystem::create_directories(folderMacro(), ec);

        std::ofstream o(pathMacro(idLevel, slot), std::ios::binary | std::ios::trunc);
        if (!o) {
            pesan = "Gagal membuat file macro";
            return false;
        }

        o.write(KODE_FILE, 4);
        tulisU32(o, static_cast<uint32_t>(g_aksi.size()));
        for (auto const& a : g_aksi) {
            tulisU32(o, a.frame);
            char sisa[2] = {
                static_cast<char>(a.tombol),
                static_cast<char>((a.tekan ? 1 : 0) | (a.pemain1 ? 2 : 0))
            };
            o.write(sisa, 2);
        }
        if (!o) {
            pesan = "Gagal menulis file macro";
            return false;
        }

        pesan = fmt::format("Slot {} tersimpan ({} aksi)", slot, g_aksi.size());
        return true;
    }

    bool muat(int idLevel, int slot, std::string& pesan) {
        auto path = pathMacro(idLevel, slot);

        std::error_code ec;
        auto ukuran = std::filesystem::file_size(path, ec);
        if (ec) {
            pesan = fmt::format("Slot {} kosong", slot);
            return false;
        }

        std::ifstream i(path, std::ios::binary);
        char kode[4];
        uint32_t jumlah = 0;
        if (!i.read(kode, 4) || !std::equal(kode, kode + 4, KODE_FILE) || !bacaU32(i, jumlah)) {
            pesan = "File macro rusak atau format tidak dikenal";
            return false;
        }

        // Cegah alokasi raksasa dari file yang dimanipulasi
        if (static_cast<std::uintmax_t>(jumlah) * UKURAN_ENTRI > ukuran - 8) {
            pesan = "File macro rusak (ukuran tidak cocok)";
            return false;
        }

        std::vector<Aksi> hasil;
        hasil.reserve(jumlah);
        for (uint32_t n = 0; n < jumlah; ++n) {
            Aksi a;
            char sisa[2];
            if (!bacaU32(i, a.frame) || !i.read(sisa, 2)) {
                pesan = "File macro terpotong";
                return false;
            }
            a.tombol = static_cast<uint8_t>(sisa[0]);
            a.tekan = (sisa[1] & 1) != 0;
            a.pemain1 = (sisa[1] & 2) != 0;
            hasil.push_back(a);
        }
        std::stable_sort(hasil.begin(), hasil.end(),
            [](Aksi const& a, Aksi const& b) { return a.frame < b.frame; });

        g_aksi = std::move(hasil);
        g_idLevel = idLevel;
        g_indeks = 0;
        g_pulihkan.clear();
        g_mode = Mode::Siaga;

        pesan = fmt::format("Slot {} dimuat ({} aksi)", slot, g_aksi.size());
        return true;
    }

    bool adaFile(int idLevel, int slot) {
        std::error_code ec;
        return std::filesystem::exists(pathMacro(idLevel, slot), ec);
    }

    bool hapus(int idLevel, int slot, std::string& pesan) {
        std::error_code ec;
        bool terhapus = std::filesystem::remove(pathMacro(idLevel, slot), ec);
        if (ec || !terhapus) {
            pesan = fmt::format("Slot {} sudah kosong", slot);
            return false;
        }
        pesan = fmt::format("Slot {} dihapus", slot);
        return true;
    }
}

#include "Keybinds.hpp"
#include "Db.hpp"
#include "../ui/MegaMenu.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <charconv>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace tgm::kb {
    namespace {
        constexpr char const* KUNCI_DB = "kb.data";

        std::vector<Item> g_bind;

        // Mode tangkap
        bool g_tangkap = false;
        std::string g_idTangkap;
        std::string g_labelTangkap;
        std::function<void()> g_selesai;

        // Katalog baris menu (dibangun sekali, dipakai untuk menjalankan aksi walau menu tertutup)
        std::vector<MegaMenu::Jendela> g_katalog;
        bool g_katalogAda = false;

        // Tombol lompat kustom yang sedang ditahan (P1, P2)
        bool g_lompatTahan[2] = {false, false};

        void kabari(std::string const& pesan, NotificationIcon ikon = NotificationIcon::None) {
            Notification::create(pesan, ikon, 1.6f)->show();
        }

        void simpan() {
            std::string s;
            for (auto const& e : g_bind) {
                s += std::to_string(e.key);
                s += '\t';
                s += e.id;
                s += '\t';
                s += e.label;
                s += '\n';
            }
            db::set(KUNCI_DB, s);
        }

        void bawaan() {
            g_bind.clear();
            g_bind.push_back({ID_MENU, "Buka Menu", static_cast<int>(KEY_Tab)});
        }

        Item* cari(std::string const& id) {
            for (auto& e : g_bind) if (e.id == id) return &e;
            return nullptr;
        }

        Item* cariKey(int key) {
            for (auto& e : g_bind) if (e.key == key) return &e;
            return nullptr;
        }

        MegaMenu::Baris* cariRekursif(std::vector<MegaMenu::Baris>& v, std::string const& id) {
            using Jenis = MegaMenu::Baris::Jenis;
            for (auto& b : v) {
                if ((b.jenis == Jenis::Toggle || b.jenis == Jenis::Tombol) && b.kunciRow == id) return &b;
                if (auto* r = cariRekursif(b.anak, id)) return r;
            }
            return nullptr;
        }

        // Jalankan toggle / tombol di menu berdasarkan id barisnya
        void jalankanAksi(std::string const& id) {
            using Jenis = MegaMenu::Baris::Jenis;
            if (!g_katalogAda) {
                g_katalog = MegaMenu::bangunJendela();
                g_katalogAda = true;
            }
            for (auto& j : g_katalog) {
                auto* b = cariRekursif(j.baris, id);
                if (!b) continue;
                if (b->jenis == Jenis::Toggle && b->ambil && b->setel) {
                    bool baru = !b->ambil();
                    b->setel(baru);
                    kabari(fmt::format("{}: {}", b->teks, baru ? "ON" : "OFF"));
                } else if (b->jenis == Jenis::Tombol && b->aksi) {
                    b->aksi();
                }
                MegaMenu::susunUlangJikaTerbuka();
                return;
            }
            kabari("Aksi keybind tidak ditemukan", NotificationIcon::Warning);
        }

        void selesaikanTangkap(int key) {
            g_tangkap = false;
            auto id = g_idTangkap;
            auto label = g_labelTangkap;
            auto selesai = std::move(g_selesai);
            g_selesai = nullptr;

            if (key == static_cast<int>(KEY_Escape)) {
                kabari("Dibatalkan");
            } else if (key == static_cast<int>(KEY_Delete) || key == static_cast<int>(KEY_Backspace)) {
                hapus(id);
                kabari(fmt::format("Keybind dihapus: {}", label), NotificationIcon::Success);
            } else {
                pasang(id, key, label);
                kabari(fmt::format("{} = {}", label, namaKunci(key)), NotificationIcon::Success);
            }
            if (selesai) selesai();
        }
    }

    // ------------------------------------------------------------------------------------------

    void muat() {
        g_bind.clear();
        if (!db::ada(KUNCI_DB)) {
            bawaan();
            simpan();
            return;
        }
        auto s = db::ambil(KUNCI_DB, "");
        std::size_t awal = 0;
        while (awal < s.size()) {
            auto akhir = s.find('\n', awal);
            if (akhir == std::string::npos) akhir = s.size();
            auto baris = s.substr(awal, akhir - awal);
            awal = akhir + 1;

            auto t1 = baris.find('\t');
            if (t1 == std::string::npos) continue;
            auto t2 = baris.find('\t', t1 + 1);
            if (t2 == std::string::npos) continue;

            int key = 0;
            auto [p, ec] = std::from_chars(baris.data(), baris.data() + t1, key);
            if (ec != std::errc{} || key <= 0) continue;
            Item e;
            e.key = key;
            e.id = baris.substr(t1 + 1, t2 - t1 - 1);
            e.label = baris.substr(t2 + 1);
            if (!e.id.empty()) g_bind.push_back(std::move(e));
        }
    }

    int tombolDari(std::string const& id) {
        auto* e = cari(id);
        return e ? e->key : 0;
    }

    void pasang(std::string const& id, int key, std::string const& label) {
        if (key <= 0) return;
        // Satu tombol hanya untuk satu aksi: pemegang lama dilepas
        for (auto it = g_bind.begin(); it != g_bind.end();) {
            if (it->key == key && it->id != id) it = g_bind.erase(it);
            else ++it;
        }
        if (auto* e = cari(id)) {
            e->key = key;
            e->label = label;
        } else {
            g_bind.push_back({id, label, key});
        }
        simpan();
    }

    void hapus(std::string const& id) {
        g_bind.erase(std::remove_if(g_bind.begin(), g_bind.end(), [&](Item const& e) { return e.id == id; }), g_bind.end());
        simpan();
    }

    void reset() {
        bawaan();
        simpan();
    }

    std::vector<Item> daftarKustom() {
        std::vector<Item> r;
        for (auto const& e : g_bind) {
            if (!e.id.empty() && e.id[0] != '@') r.push_back(e);
        }
        return r;
    }

    std::string namaKunci(int key) {
        if (key <= 0) return "-";
        switch (static_cast<enumKeyCodes>(key)) {
            case KEY_Tab: return "Tab";
            case KEY_Space: return "Space";
            case KEY_Enter: return "Enter";
            case KEY_Escape: return "Esc";
            case KEY_Backspace: return "Bksp";
            case KEY_Delete: return "Del";
            case KEY_Up: return "Atas";
            case KEY_Down: return "Bawah";
            case KEY_Left: return "Kiri";
            case KEY_Right: return "Kanan";
            case KEY_Shift: return "Shift";
            case KEY_Control: return "Ctrl";
            default: break;
        }
        switch (key) {   // kode virtual-key standar
            case 18: return "Alt";
            case 33: return "PgUp";
            case 34: return "PgDn";
            case 35: return "End";
            case 36: return "Home";
            case 45: return "Ins";
            default: break;
        }
        if (key >= 65 && key <= 90) return std::string(1, static_cast<char>(key));        // A - Z
        if (key >= 48 && key <= 57) return std::string(1, static_cast<char>(key));        // 0 - 9
        if (key >= 112 && key <= 123) return fmt::format("F{}", key - 111);               // F1 - F12
        if (key >= 96 && key <= 105) return fmt::format("Num{}", key - 96);               // Numpad 0 - 9
        return fmt::format("Key{}", key);
    }

    void mulaiTangkap(std::string const& id, std::string const& label, std::function<void()> selesai) {
        g_tangkap = true;
        g_idTangkap = id;
        g_labelTangkap = label;
        g_selesai = std::move(selesai);
        kabari(fmt::format("Tekan tombol untuk: {}  (Esc batal, Del hapus)", label));
    }

    void batalTangkap() {
        if (!g_tangkap) return;
        g_tangkap = false;
        auto selesai = std::move(g_selesai);
        g_selesai = nullptr;
        kabari("Dibatalkan");
        if (selesai) selesai();
    }

    bool sedangMenangkap() { return g_tangkap; }

    bool sedangMenangkap(std::string const& id) { return g_tangkap && g_idTangkap == id; }

    bool tangani(int key, bool down, bool repeat) {
        if (key <= 0) return false;

        // Mode tangkap: tombol pertama yang ditekan dipasang
        if (g_tangkap) {
            if (down && !repeat) selesaikanTangkap(key);
            return true;
        }

        auto* e = cariKey(key);
        if (!e) return false;
        Item salinan = *e;   // aksi bisa mengubah daftar bind

        // Buka / tutup menu: berlaku di mana saja
        if (salinan.id == ID_MENU) {
            if (down && !repeat) MegaMenu::bukaAtauTutup();
            return true;
        }

        auto* pl = PlayLayer::get();

        // Lompat kustom P1 / P2: meniru sentuhan tombol lompat
        if (salinan.id == ID_P1 || salinan.id == ID_P2) {
            int i = salinan.id == ID_P1 ? 0 : 1;
            if (!down) {
                if (g_lompatTahan[i]) {
                    g_lompatTahan[i] = false;
                    if (pl) pl->handleButton(false, 1, i == 0);
                    return true;
                }
                return false;
            }
            if (!pl || pl->m_isPaused || MegaMenu::terbuka()) return false;
            if (repeat) return true;
            g_lompatTahan[i] = true;
            pl->handleButton(true, 1, i == 0);
            return true;
        }

        // Toggle / tombol menu: hanya di dalam level atau saat menu terbuka (agar tidak terpicu saat mengetik di tempat lain)
        if (!pl && !MegaMenu::terbuka()) return false;
        if (down && !repeat) jalankanAksi(salinan.id);
        return true;
    }
}

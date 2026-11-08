#include "Config.hpp"
#include "Db.hpp"
#include "Riwayat.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace tgm {
    namespace {
        // Satu pengaturan: kunci di database, penunjuk ke field, rentang nilai, dan nama kunci lama
        // (saved value Geode versi sebelumnya) untuk migrasi otomatis.
        struct Entri {
            char const* kunci;
            char        tipe;      // 'b' bool, 'i' int, 'f' float
            void*       ptr;
            double      mn;
            double      mx;
            char const* lama;      // nullptr bila tidak ada di versi lama
        };

        std::vector<Entri> const& daftar() {
            static std::vector<Entri> v = [] {
                auto& c = cfg();
                std::vector<Entri> r;
                auto B = [&](char const* k, bool& f, char const* lama = nullptr) {
                    r.push_back({k, 'b', &f, 0, 1, lama});
                };
                auto I = [&](char const* k, int& f, int mn, int mx, char const* lama = nullptr) {
                    r.push_back({k, 'i', &f, static_cast<double>(mn), static_cast<double>(mx), lama});
                };
                auto F = [&](char const* k, float& f, float mn, float mx, char const* lama = nullptr) {
                    r.push_back({k, 'f', &f, mn, mx, lama});
                };

                // Hack
                B("noclip", c.noclip, "noclip");
                B("noclip-p1", c.noclipP1, "noclip-p1");
                B("noclip-p2", c.noclipP2, "noclip-p2");
                I("batas-tabrakan", c.batasTabrakan, 0, 100, "batas-tabrakan");
                I("akurasi-min", c.akurasiMin, 0, 99);
                B("auto-safe-mode", c.autoSafeMode, "auto-safe-mode");
                B("no-death-effect", c.noDeathEffect, "no-death-effect");
                B("respawn-cepat", c.respawnCepat);
                I("respawn-ms", c.respawnMs, 0, 1000);
                B("auto-checkpoint", c.autoCheckpoint, "auto-checkpoint");
                I("checkpoint-dtk", c.checkpointDtk, 1, 30, "checkpoint-dtk");
                F("speed", c.speed, 0.1f, 3.f, "speed");
                B("lompat-bebas", c.lompatBebas);
                B("tanpa-guncang", c.tanpaGuncang);
                B("tanpa-cermin", c.tanpaCermin);
                B("buka-semua", c.bukaSemua);
                B("jeda-saat-buka", c.jedaSaatBuka);

                // HUD isi
                B("show-hud", c.showHud, "show-hud");
                B("show-fps", c.showFps, "show-fps");
                B("show-cps", c.showCps);
                B("show-clicks", c.showClicks, "show-clicks");
                B("show-clock", c.showClock, "show-clock");
                B("show-percent", c.showPercent, "show-percent");
                B("show-attempt", c.showAttempt, "show-attempt");
                B("show-frame", c.showFrame, "show-frame");
                B("show-time", c.showTime, "show-time");
                B("show-deaths", c.showDeaths, "show-deaths");
                B("show-best", c.showBest);
                B("show-level-name", c.showLevelName);
                B("show-akurasi", c.showAkurasi);

                // HUD gaya
                I("hud-opacity", c.hudOpacity, 10, 100, "hud-opacity");
                I("hud-scale", c.hudScale, 50, 150, "hud-scale");
                I("hud-corner", c.hudCorner, 0, 5, "hud-corner");
                I("hud-color", c.hudColor, 0, 7, "hud-color");
                I("hud-font", c.hudFont, 0, 2);
                B("hud-panel", c.hudPanel);
                I("hud-off-x", c.hudOffX, -100, 100);
                I("hud-off-y", c.hudOffY, -100, 100);

                // Bot
                I("slot", c.slot, 1, 5, "slot");
                B("auto-save-macro", c.autoSaveMacro, "auto-save-macro");
                B("auto-play", c.autoPlay, "auto-play");
                B("auto-muat", c.autoMuat);

                // Logo
                B("logo-tampil", c.logoTampil);
                B("logo-di-level", c.logoDiLevel);
                B("logo-magnet", c.logoMagnet);
                B("logo-redup", c.logoRedup);
                I("logo-ukuran", c.logoUkuran, 50, 200);
                I("logo-opasitas", c.logoOpasitas, 20, 100);
                F("logo-x", c.logoX, 0.f, 1.f);
                F("logo-y", c.logoY, 0.f, 1.f);

                // Musik practice
                B("musik-practice-asli", c.musikPracticeAsli);

                // Input viewer
                B("iv-tampil", c.ivTampil);
                I("iv-posisi", c.ivPosisi, 0, 5);
                I("iv-ukuran", c.ivUkuran, 50, 200);
                I("iv-opasitas", c.ivOpasitas, 20, 100);
                I("iv-warna", c.ivWarna, 0, 7);
                I("iv-off-x", c.ivOffX, -150, 150);
                I("iv-off-y", c.ivOffY, -150, 150);

                // Presisi klik
                I("cbf-mode", c.cbfMode, 0, 2);
                B("turbo", c.turbo);
                I("turbo-cps", c.turboCps, 5, 30);

                // Tambahan
                B("konfirmasi-keluar", c.konfirmasiKeluar);
                B("tombol-restart", c.tombolRestart);
                B("show-mati-terakhir", c.showMatiTerakhir);
                B("hitbox", c.hitbox);
                B("hitbox-mati", c.hitboxMati);
                B("frame-stepper", c.frameStepper);

                // Baru di v1.6.0
                B("mode-ringan", c.modeRingan);
                B("tanpa-partikel", c.tanpaPartikel);
                B("auto-practice", c.autoPractice);
                B("fps-bypass", c.fpsBypass);
                I("fps-target", c.fpsTarget, 30, 1000);
                B("lintasan", c.lintasan);
                I("lintasan-langkah", c.lintasanLangkah, 20, 120);

                // Antarmuka
                I("tab-terakhir", c.tabTerakhir, 0, 20);
                I("tema", c.tema, 0, 7);
                B("menu-terang", c.menuTerang);
                I("menu-skala", c.menuSkala, 60, 130);
                I("menu-opasitas", c.menuOpasitas, 40, 100);
                I("panel-tutup", c.panelTutup, 0, 65535);
                return r;
            }();
            return v;
        }

        std::string kunciDb(Entri const& e) { return std::string("cfg.") + e.kunci; }

        // Tempat nilai bawaan: objek Config baru dengan offset field yang sama
        void* ptrBawaan(Config& bawaan, Entri const& e) {
            auto selisih = reinterpret_cast<char*>(e.ptr) - reinterpret_cast<char*>(&cfg());
            return reinterpret_cast<char*>(&bawaan) + selisih;
        }

        void terapkanRentang(Entri const& e) {
            if (e.tipe == 'i') {
                auto* p = static_cast<int*>(e.ptr);
                *p = std::clamp(*p, static_cast<int>(e.mn), static_cast<int>(e.mx));
            } else if (e.tipe == 'f') {
                auto* p = static_cast<float*>(e.ptr);
                if (!std::isfinite(*p)) *p = static_cast<float>(e.mn);
                *p = std::clamp(*p, static_cast<float>(e.mn), static_cast<float>(e.mx));
            }
        }
    }

    Config& cfg() {
        static Config s_config;
        return s_config;
    }

    void loadConfig() {
        auto* mod = Mod::get();
        Config bawaan;

        // Pertama kali setelah pembaruan: ambil pengaturan dari penyimpanan lama (saved value Geode)
        bool migrasi = !db::ada("cfg.versi");

        for (auto const& e : daftar()) {
            auto kunci = kunciDb(e);
            bool ada = db::ada(kunci);
            auto* asal = ptrBawaan(bawaan, e);

            switch (e.tipe) {
                case 'b': {
                    bool nilai = *static_cast<bool*>(asal);
                    if (ada) nilai = db::ambilBool(kunci, nilai);
                    else if (migrasi && e.lama && mod->hasSavedValue(e.lama)) nilai = mod->getSavedValue<bool>(e.lama, nilai);
                    *static_cast<bool*>(e.ptr) = nilai;
                    break;
                }
                case 'i': {
                    int nilai = *static_cast<int*>(asal);
                    if (ada) nilai = static_cast<int>(db::ambilInt(kunci, nilai));
                    else if (migrasi && e.lama && mod->hasSavedValue(e.lama)) nilai = mod->getSavedValue<int>(e.lama, nilai);
                    *static_cast<int*>(e.ptr) = nilai;
                    break;
                }
                case 'f': {
                    float nilai = *static_cast<float*>(asal);
                    if (ada) nilai = static_cast<float>(db::ambilDbl(kunci, nilai));
                    else if (migrasi && e.lama && mod->hasSavedValue(e.lama)) {
                        nilai = static_cast<float>(mod->getSavedValue<double>(e.lama, nilai));
                    }
                    *static_cast<float*>(e.ptr) = nilai;
                    break;
                }
            }
            terapkanRentang(e);
        }

        if (migrasi) {
            db::setInt("cfg.versi", 1, false);
            saveConfig();
        }
    }

    void saveConfig() {
        db::tahan();
        for (auto const& e : daftar()) {
            terapkanRentang(e);
            auto kunci = kunciDb(e);
            switch (e.tipe) {
                case 'b': db::setBool(kunci, *static_cast<bool*>(e.ptr)); break;
                case 'i': db::setInt(kunci, *static_cast<int*>(e.ptr)); break;
                case 'f': db::setDbl(kunci, static_cast<double>(*static_cast<float*>(e.ptr))); break;
            }
        }
        db::setInt("cfg.versi", 1);
        db::lepas();
    }

    void resetConfig() {
        cfg() = Config{};
        saveConfig();
    }
}

// Buka database lalu muat pengaturan setelah data simpanan mod selesai dibaca
$on_mod(DataLoaded) {
    tgm::db::muat();
    tgm::loadConfig();
    // Catat sesi game baru setelah database terbuka
    queueInMainThread([] { tgm::riwayat::catatSesiBaru(); });
}

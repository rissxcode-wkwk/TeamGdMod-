// Hook PlayLayer: Noclip, Mode Aman, HUD, checkpoint otomatis, dan siklus hidup Replay Bot.

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <string>

#include "../bot/Bot.hpp"
#include "../core/Cheats.hpp"
#include "../core/Config.hpp"
#include "../core/Stats.hpp"

using namespace geode::prelude;

namespace {
    // Jam saat ini dalam format HH:MM
    std::string jamSekarang() {
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm lokal{};
#if defined(GEODE_IS_WINDOWS)
        localtime_s(&lokal, &t);
#else
        localtime_r(&t, &lokal);
#endif
        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M", &lokal);
        return buf;
    }

    // Menyusun teks HUD dari status saat ini. Setiap baris hanya muncul bila diaktifkan / relevan.
    std::string susunTeksHud(PlayLayer* pl, bool modeAman) {
        auto const& c = tgm::cfg();
        auto const& s = tgm::stats();
        std::string t;

        if (c.showFps)     t += fmt::format("FPS: {:.0f}\n", s.fps);
        if (c.showClock)   t += fmt::format("Jam: {}\n", jamSekarang());
        if (c.showPercent) t += fmt::format("Persen: {:.1f}%\n", pl->getCurrentPercent());
        if (c.showAttempt) t += fmt::format("Percobaan: {}\n", pl->m_attempts);
        if (c.showTime)    t += fmt::format("Waktu: {:.1f} dtk\n", pl->m_gameState.m_levelTime);
        if (c.showFrame)   t += fmt::format("Frame: {}\n", pl->m_gameState.m_currentProgress);
        if (c.showClicks)  t += fmt::format("Klik: {}\n", s.klik);
        if (c.showDeaths)  t += fmt::format("Mati (sesi): {}\n", tgm::sesi().mati);
        if (c.noclip) {
            if (c.batasTabrakan > 0) {
                t += fmt::format("Noclip: {}/{} tabrakan\n", s.tabrakan, c.batasTabrakan);
            } else {
                t += fmt::format("Noclip: {} tabrakan\n", s.tabrakan);
            }
        }
        if (std::abs(c.speed - 1.f) > 0.001f) {
            t += fmt::format("Kecepatan: {:.1f}x\n", c.speed);
        }

        switch (tgm::bot::mode()) {
            case tgm::bot::Mode::Rekam:
                t += fmt::format("Bot: Merekam ({} aksi)\n", tgm::bot::jumlahAksi());
                break;
            case tgm::bot::Mode::Putar:
                t += fmt::format("Bot: Memutar ({}/{})\n", tgm::bot::posisi(), tgm::bot::jumlahAksi());
                break;
            default:
                break;
        }

        if (modeAman) t += "Mode Aman aktif\n";
        if (!t.empty()) t.pop_back(); // buang newline terakhir
        return t;
    }

    // Warna HUD yang bisa dipilih (indeks = Config::hudColor)
    ccColor3B warnaHud(int indeks) {
        static ccColor3B const daftar[] = {
            {255, 255, 255}, // putih
            {56, 208, 255},  // cyan
            {61, 220, 151},  // hijau
            {255, 209, 102}, // kuning
            {255, 99, 99},   // merah
        };
        return daftar[std::clamp(indeks, 0, 4)];
    }

    // Menempatkan label HUD sesuai sudut, ukuran, warna, dan kepekatan pilihan
    void terapkanGayaHud(CCLabelBMFont* label) {
        auto const& c = tgm::cfg();
        auto win = CCDirector::get()->getWinSize();

        bool kanan = (c.hudCorner == 1 || c.hudCorner == 3);
        bool bawah = (c.hudCorner == 2 || c.hudCorner == 3);

        label->setAnchorPoint({kanan ? 1.f : 0.f, bawah ? 0.f : 1.f});
        label->setAlignment(kanan ? kCCTextAlignmentRight : kCCTextAlignmentLeft);
        label->setPosition({kanan ? win.width - 8.f : 8.f, bawah ? 6.f : win.height - 6.f});
        label->setScale(0.55f * static_cast<float>(c.hudScale) / 100.f);
        label->setColor(warnaHud(c.hudColor));
        label->setOpacity(static_cast<GLubyte>(std::clamp(c.hudOpacity, 10, 100) * 255 / 100));
    }

    void kabari(std::string const& pesan, NotificationIcon ikon) {
        Notification::create(pesan, ikon, 1.8f)->show();
    }
}

class $modify(TgmPlayLayer, PlayLayer) {
    struct Fields {
        std::string teksTerakhir;   // cache agar setString tidak dipanggil tiap frame
        float       akumulasiHud = 0.f;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        auto& st = tgm::stats();
        st = tgm::Stats{};

        // Ikat bot ke level ini (macro level lain dibuang)
        tgm::bot::ikatLevel(m_level->m_levelID.value());

        // Mode Aman: cheat aktif -> progres tidak disimpan
        if (tgm::cfg().autoSafeMode && tgm::cheatsActive()) {
            m_isTestMode = true;
        }

        // Label HUD di pojok kiri atas
        auto label = CCLabelBMFont::create("", "chatFont.fnt");
        label->setID("tgm-hud"_spr);
        terapkanGayaHud(label);
        m_uiLayer->addChild(label, 100);
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        auto& st = tgm::stats();
        st.klik = 0;
        st.tabrakan = 0;
        st.detikCp = 0.f;

        // Setelah respawn, sinkronkan bot dengan frame checkpoint
        auto frame = m_gameState.m_currentProgress;
        switch (tgm::bot::mode()) {
            case tgm::bot::Mode::Rekam: tgm::bot::potongDari(frame); break;
            case tgm::bot::Mode::Putar: tgm::bot::lompatKe(frame);   break;
            default:
                // Putar Otomatis: mulai dari awal level bila macro sudah dimuat
                if (tgm::cfg().autoPlay && frame == 0 && tgm::bot::jumlahAksi() > 0) {
                    tgm::bot::mulaiPutar();
                }
                break;
        }

        // Mode Aman diterapkan lagi (cheat bisa saja baru dinyalakan lewat menu)
        if (tgm::cfg().autoSafeMode && tgm::cheatsActive()) {
            m_isTestMode = true;
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto const& c = tgm::cfg();

        // Spike anti-cheat bawaan GD tetap harus diproses agar level tidak error
        if (c.noclip && object != m_anticheatSpike) {
            bool pemain1 = (player == m_player1);
            bool berlaku = pemain1 ? c.noclipP1 : c.noclipP2;
            if (berlaku) {
                auto& st = tgm::stats();
                st.tabrakan++;
                // Batas tabrakan: setelah terlampaui, pemain mati seperti biasa
                bool lewatBatas = (c.batasTabrakan > 0 && st.tabrakan > static_cast<uint32_t>(c.batasTabrakan));
                if (!lewatBatas) return;
            }
        }

        tgm::sesi().mati++;
        PlayLayer::destroyPlayer(player, object);
    }

    void levelComplete() {
        // Mode Aman: jangan simpan penyelesaian bila ada cheat aktif
        if (tgm::cfg().autoSafeMode && tgm::cheatsActive()) {
            m_isTestMode = true;
        }

        // Auto-simpan rekaman ke slot terpilih
        if (tgm::bot::mode() == tgm::bot::Mode::Rekam && tgm::cfg().autoSaveMacro) {
            std::string pesan;
            bool ok = tgm::bot::simpan(m_level->m_levelID.value(), tgm::cfg().slot, pesan);
            kabari(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
        }

        tgm::bot::berhenti();
        PlayLayer::levelComplete();
    }

    void onQuit() {
        tgm::bot::berhenti();
        PlayLayer::onQuit();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto const& c = tgm::cfg();
        auto& st = tgm::stats();

        // FPS terhaluskan (dt sudah diskalakan Speedhack, jadi dikembalikan ke waktu nyata)
        float dtNyata = dt / std::max(c.speed, 0.01f);
        if (dtNyata > 0.0001f) {
            st.fps = st.fps * 0.9f + (1.f / dtNyata) * 0.1f;
        }

        // Checkpoint otomatis: hanya di mode practice, saat pemain hidup dan level belum selesai
        if (c.autoCheckpoint && m_isPracticeMode && !m_levelEndAnimationStarted
            && m_player1 && !m_player1->m_isDead) {
            st.detikCp += dtNyata;
            if (st.detikCp >= static_cast<float>(c.checkpointDtk)) {
                st.detikCp = 0.f;
                this->markCheckpoint();
            }
        }

        // Perbarui HUD paling banyak 10x per detik
        m_fields->akumulasiHud += dtNyata;
        if (m_fields->akumulasiHud < 0.1f) return;
        m_fields->akumulasiHud = 0.f;

        auto* label = typeinfo_cast<CCLabelBMFont*>(m_uiLayer->getChildByID("tgm-hud"_spr));
        if (!label) return;

        if (!c.showHud) {
            label->setVisible(false);
            return;
        }
        label->setVisible(true);
        terapkanGayaHud(label);

        auto teks = susunTeksHud(this, m_isTestMode && tgm::cheatsActive());
        if (teks != m_fields->teksTerakhir) {
            m_fields->teksTerakhir = teks;
            label->setString(teks.c_str());
        }
    }
};

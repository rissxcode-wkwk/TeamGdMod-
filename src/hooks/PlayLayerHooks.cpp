// Hook PlayLayer: Noclip, Mode Aman otomatis, HUD status, dan siklus hidup Replay Bot.

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <cmath>
#include <string>

#include "../bot/Bot.hpp"
#include "../core/Cheats.hpp"
#include "../core/Config.hpp"

using namespace geode::prelude;

namespace {
    // Menyusun teks HUD dari status saat ini
    std::string susunTeksHud(int tabrakan, bool modeAman) {
        std::string t;
        auto const& c = tgm::cfg();

        if (c.noclip) {
            t += fmt::format("Noclip: {} tabrakan\n", tabrakan);
        }
        if (std::abs(c.speed - 1.f) > 0.001f) {
            t += fmt::format("Kecepatan: {:.2f}x\n", c.speed);
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
        if (modeAman) {
            t += "Mode Aman aktif\n";
        }
        if (!t.empty()) t.pop_back(); // buang newline terakhir
        return t;
    }
}

class $modify(TgmPlayLayer, PlayLayer) {
    struct Fields {
        int         tabrakan = 0;   // jumlah tabrakan yang diabaikan Noclip (per percobaan)
        std::string teksTerakhir;   // cache agar setString tidak dipanggil tiap frame
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        // Ikat bot ke level ini (macro level lain dibuang)
        tgm::bot::ikatLevel(m_level->m_levelID.value());

        // Mode Aman: cheat aktif -> progres tidak disimpan
        if (tgm::cfg().autoSafeMode && tgm::cheatsActive()) {
            m_isTestMode = true;
        }

        // Label HUD di pojok kiri atas
        auto label = CCLabelBMFont::create("", "chatFont.fnt");
        label->setID("tgm-hud"_spr);
        label->setAnchorPoint({0.f, 1.f});
        label->setScale(0.55f);
        label->setOpacity(170);
        label->setAlignment(kCCTextAlignmentLeft);
        auto winSize = CCDirector::get()->getWinSize();
        label->setPosition({8.f, winSize.height - 6.f});
        m_uiLayer->addChild(label, 100);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->tabrakan = 0;

        // Setelah respawn, sinkronkan bot dengan frame checkpoint
        auto frame = m_gameState.m_currentProgress;
        switch (tgm::bot::mode()) {
            case tgm::bot::Mode::Rekam: tgm::bot::potongDari(frame); break;
            case tgm::bot::Mode::Putar: tgm::bot::lompatKe(frame);   break;
            default: break;
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        // Spike anti-cheat bawaan GD tetap harus diproses agar level tidak error
        if (tgm::cfg().noclip && object != m_anticheatSpike) {
            m_fields->tabrakan++;
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void levelComplete() {
        // Mode Aman: jangan simpan penyelesaian bila ada cheat aktif
        if (tgm::cfg().autoSafeMode && tgm::cheatsActive()) {
            m_isTestMode = true;
        }
        // Rekaman/pemutaran selesai bersama level (macro tetap di memori)
        tgm::bot::berhenti();
        PlayLayer::levelComplete();
    }

    void onQuit() {
        tgm::bot::berhenti();
        PlayLayer::onQuit();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto* node = m_uiLayer->getChildByID("tgm-hud"_spr);
        auto* label = typeinfo_cast<CCLabelBMFont*>(node);
        if (!label) return;

        if (!tgm::cfg().showHud) {
            label->setVisible(false);
            return;
        }
        label->setVisible(true);

        auto teks = susunTeksHud(m_fields->tabrakan, m_isTestMode && tgm::cheatsActive());
        if (teks != m_fields->teksTerakhir) {
            m_fields->teksTerakhir = teks;
            label->setString(teks.c_str());
        }
    }
};

// Hook input untuk Replay Bot: merekam tombol, menyuntikkannya saat pemutaran, dan menghitung klik.

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include "../bot/Bot.hpp"
#include "../core/Stats.hpp"

using namespace geode::prelude;

namespace {
    // true selama kita sendiri yang memanggil handleButton (supaya tidak ikut direkam / dihitung / diblokir)
    bool g_menyuntik = false;

    // Hanya layar bermain (PlayLayer) yang diproses, bukan editor
    bool layarBermain(GJBaseGameLayer* self) {
        auto* pl = PlayLayer::get();
        return pl && static_cast<GJBaseGameLayer*>(pl) == self;
    }
}

class $modify(TgmBotBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (!layarBermain(this) || g_menyuntik) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        switch (tgm::bot::mode()) {
            case tgm::bot::Mode::Rekam:
                tgm::bot::catat(m_gameState.m_currentProgress, button, down, isPlayer1);
                break;
            case tgm::bot::Mode::Putar:
                // Selama pemutaran, input manual diabaikan agar tidak merusak macro
                if (tgm::bot::sedangMemutar()) return;
                break;
            default:
                break;
        }

        if (down) {
            tgm::stats().klik++;
            tgm::sesi().klik++;
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        if (tgm::bot::mode() == tgm::bot::Mode::Putar && layarBermain(this)) {
            auto frame = m_gameState.m_currentProgress;
            while (auto aksi = tgm::bot::ambil(frame)) {
                g_menyuntik = true;
                this->handleButton(aksi->tekan, aksi->tombol, aksi->pemain1);
                g_menyuntik = false;
                if (aksi->tekan) { tgm::stats().klik++; tgm::sesi().klik++; }
            }
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

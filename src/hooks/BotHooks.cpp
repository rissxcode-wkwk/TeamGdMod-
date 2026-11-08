// Hook input: Replay Bot (rekam / putar), statistik klik,
// klik otomatis (turbo), dan penghubung ke presisi klik (CBF).

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>
#include <cmath>

#include "../bot/Bot.hpp"
#include "../core/Config.hpp"
#include "../core/Riwayat.hpp"
#include "../core/Stats.hpp"
#include "../hacks/Cbf.hpp"

using namespace geode::prelude;

namespace {
    // true selama kita sendiri yang memanggil handleButton (supaya tidak ikut direkam / dihitung / diblokir)
    bool g_menyuntik = false;

    // Tombol lompat yang sedang ditahan pengguna (indeks 0 = P1, 1 = P2) dan status klik otomatis
    bool g_tahan[2] = {false, false};
    bool g_keadaanTurbo[2] = {false, false};
    uint32_t g_frameTurbo[2] = {0, 0};

    // Satu klik: statistik percobaan, CPS, sesi, dan riwayat permanen
    void catatKlik() {
        auto& st = tgm::stats();
        st.klik++;
        st.waktuKlik.push_back(st.waktuNyata);
        tgm::sesi().klik++;
        if (auto* pl = PlayLayer::get(); pl && pl->m_level) {
            tgm::riwayat::catatKlik(pl->m_level->m_levelID.value());
        }
    }

    // Hanya layar bermain (PlayLayer) yang diproses, bukan editor
    bool layarBermain(GJBaseGameLayer* self) {
        auto* pl = PlayLayer::get();
        return pl && static_cast<GJBaseGameLayer*>(pl) == self;
    }
}

class $modify(TgmBotBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (!layarBermain(this)) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }
        if (g_menyuntik) {
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

        if (button == 1) {
            int i = isPlayer1 ? 0 : 1;
            g_tahan[i] = down;
            g_keadaanTurbo[i] = down;
        }
        if (down) catatKlik();
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        if (layarBermain(this)) {
            // Presisi klik (mode Klik di Langkah) memproses antrean input lebih dulu
            tgm::cbf::sebelumProcessCommands();

            if (tgm::bot::mode() == tgm::bot::Mode::Putar) {
                auto frame = m_gameState.m_currentProgress;
                while (auto aksi = tgm::bot::ambil(frame)) {
                    g_menyuntik = true;
                    this->handleButton(aksi->tekan, aksi->tombol, aksi->pemain1);
                    g_menyuntik = false;
                    if (aksi->tekan) catatKlik();
                }
            } else if (tgm::cfg().turbo && tgm::bot::mode() == tgm::bot::Mode::Siaga) {
                // Klik otomatis: selama tombol lompat ditahan, tekan dan lepas bergantian
                uint32_t frame = m_gameState.m_currentProgress;
                int paruh = std::max(1, static_cast<int>(std::lround(240.0 / (2.0 * std::max(1, tgm::cfg().turboCps)))));
                for (int i = 0; i < 2; ++i) {
                    if (frame < g_frameTurbo[i]) { g_tahan[i] = false; g_keadaanTurbo[i] = false; }   // level diulang
                    if (!g_tahan[i] || frame == g_frameTurbo[i] || frame % static_cast<uint32_t>(paruh) != 0) continue;
                    g_frameTurbo[i] = frame;
                    g_keadaanTurbo[i] = !g_keadaanTurbo[i];
                    g_menyuntik = true;
                    this->handleButton(g_keadaanTurbo[i], 1, i == 0);
                    g_menyuntik = false;
                    if (g_keadaanTurbo[i]) catatKlik();
                }
            }
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

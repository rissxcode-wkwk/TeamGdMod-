// Hack tampilan dan alat bantu pada GJBaseGameLayer:
// Tanpa Guncangan, Tanpa Portal Cermin, pengaman debug draw (Hitbox), dan Frame Stepper.

#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>

#include "../core/Config.hpp"
#include "Stepper.hpp"

using namespace geode::prelude;

namespace tgm::stepper {
    namespace {
        bool g_beku = false;
        int  g_langkah = 0;
    }

    bool beku() { return g_beku; }

    void ubahBeku() {
        g_beku = !g_beku;
        g_langkah = 0;
        // Musik level ikut berhenti saat beku supaya tidak jalan sendiri
        if (g_beku) FMODAudioEngine::get()->pauseMusic(0);
        else FMODAudioEngine::get()->resumeMusic(0);
    }

    void langkah() { if (g_beku && g_langkah < 8) ++g_langkah; }

    bool ambilLangkah() {
        if (g_langkah <= 0) return false;
        --g_langkah;
        return true;
    }

    void reset() {
        // Keluar level saat beku: kembalikan musik agar menu tidak sunyi
        if (g_beku) FMODAudioEngine::get()->resumeMusic(0);
        g_beku = false;
        g_langkah = 0;
    }
}

class $modify(TgmVisualLayer, GJBaseGameLayer) {
    void shakeCamera(float durasi, float kekuatan, float interval) {
        if (tgm::cfg().tanpaGuncang || tgm::cfg().modeRingan) return;
        GJBaseGameLayer::shakeCamera(durasi, kekuatan, interval);
    }

    void toggleFlipped(bool balik, bool tanpaEfek) {
        if (tgm::cfg().tanpaCermin) return;
        GJBaseGameLayer::toggleFlipped(balik, tgm::cfg().instantMirror ? true : tanpaEfek);
    }

    // Pengaman: debug draw (hitbox) tidak boleh berjalan tanpa node gambarnya
    void updateDebugDraw() {
        if (!m_debugDrawNode) return;
        GJBaseGameLayer::updateDebugDraw();
    }

    // Frame Stepper: saat beku, fisika hanya maju satu langkah per permintaan
    void update(float dt) {
        auto* pl = PlayLayer::get();
        if (pl && static_cast<GJBaseGameLayer*>(pl) == this && tgm::stepper::beku()) {
            if (!tgm::cfg().frameStepper) {
                tgm::stepper::ubahBeku();   // fitur dimatikan saat beku: lanjutkan level
            } else {
                if (!tgm::stepper::ambilLangkah()) return;
                float tw = std::min(1.f, m_gameState.m_timeWarp);
                GJBaseGameLayer::update(tw / 240.f);   // tepat satu langkah fisika
                return;
            }
        }
        GJBaseGameLayer::update(dt);
    }
};

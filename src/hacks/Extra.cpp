// FPS Bypass: mengatur interval animasi cocos2d sesuai FPS target pilihan pengguna.
// Hasil akhirnya tetap bergantung pada perangkat (layar, V-Sync, pembatas dari GD).

#include "Extra.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>

#include "../core/Config.hpp"

using namespace geode::prelude;

namespace tgm {
    namespace {
        bool g_fpsDiubah = false;   // true bila kita pernah mengubah interval (agar bisa dikembalikan)
    }

    void terapkanFps() {
        auto const& c = cfg();
        auto* app = CCApplication::sharedApplication();
        if (!app) return;

        if (c.fpsBypass) {
            double target = static_cast<double>(std::clamp(c.fpsTarget, 30, 360));
            app->setAnimationInterval(1.0 / target);
            g_fpsDiubah = true;
        } else if (g_fpsDiubah) {
            app->setAnimationInterval(1.0 / 60.0);
            g_fpsDiubah = false;
        }
    }
}

// FPS Bypass: mengatur interval animasi cocos2d sesuai target FPS pilihan.
// Eksperimental. File ini berdiri sendiri: kalau build gagal gara-gara file ini, hapus saja.

#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "../core/Config.hpp"

using namespace geode::prelude;

class $modify(TgmFpsScheduler, CCScheduler) {
    void update(float dt) {
        static double s_terakhir = -1.0;
        auto const& c = tgm::cfg();
        double mau = c.fpsBypass ? 1.0 / static_cast<double>(c.fpsTarget) : 0.0;
        if (mau != s_terakhir) {
            // Pertama kali (-1) dan bypass mati: jangan diutak-atik, biarkan pengaturan GD
            if (c.fpsBypass) {
                CCDirector::get()->setAnimationInterval(mau);
            } else if (s_terakhir > 0.0) {
                CCDirector::get()->setAnimationInterval(1.0 / 60.0);   // kembali ke bawaan
            }
            s_terakhir = mau;
        }
        CCScheduler::update(dt);
    }
};

// Speedhack: skala waktu game lewat CCScheduler. Hanya aktif saat bermain di dalam level.
// Hook ini juga mencatat waktu awal tiap frame untuk presisi klik (CBF).

#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "../core/Config.hpp"
#include "Cbf.hpp"

using namespace geode::prelude;

class $modify(TgmScheduler, CCScheduler) {
    void update(float dt) {
        tgm::cbf::awalFrame();
        // Hanya saat level berjalan: layar jeda dan menu tetap berjalan normal
        if (auto* pl = PlayLayer::get(); pl && !pl->m_isPaused) {
            dt *= tgm::cfg().speed;
        }
        CCScheduler::update(dt);
    }
};

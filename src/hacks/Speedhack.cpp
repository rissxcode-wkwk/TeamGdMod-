// Speedhack: skala waktu game lewat CCScheduler. Hanya aktif saat bermain di dalam level.

#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "../core/Config.hpp"

using namespace geode::prelude;

class $modify(TgmScheduler, CCScheduler) {
    void update(float dt) {
        if (PlayLayer::get()) {
            dt *= tgm::cfg().speed;
        }
        CCScheduler::update(dt);
    }
};

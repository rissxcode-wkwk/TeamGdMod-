// Tanpa Efek Kematian: melewati animasi ledakan pemain saat mati.

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include "../core/Config.hpp"

using namespace geode::prelude;

class $modify(TgmPlayerObject, PlayerObject) {
    void playDeathEffect() {
        if (tgm::cfg().noDeathEffect || tgm::cfg().modeRingan) return;
        PlayerObject::playDeathEffect();
    }
};

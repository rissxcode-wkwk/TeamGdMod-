// Hook keyboard: meneruskan setiap penekanan tombol ke sistem keybind TeamGdMod.

#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

#include "../core/Keybinds.hpp"

using namespace geode::prelude;

class $modify(TgmKeyboard, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool isKeyRepeat, double timestamp) {
        if (tgm::kb::tangani(static_cast<int>(key), down, isKeyRepeat)) return true;
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, isKeyRepeat, timestamp);
    }
};

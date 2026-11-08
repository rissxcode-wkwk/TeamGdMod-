// Buka Semua: semua ikon dan warna dianggap terbuka (hanya tampilan lokal, tidak mengubah akun).

#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>

#include "../core/Config.hpp"

using namespace geode::prelude;

class $modify(TgmGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (tgm::cfg().bukaSemua) return true;
        return GameManager::isIconUnlocked(id, type);
    }

    bool isColorUnlocked(int id, UnlockType type) {
        if (tgm::cfg().bukaSemua) return true;
        return GameManager::isColorUnlocked(id, type);
    }
};

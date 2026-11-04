#include "Config.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>

using namespace geode::prelude;

namespace tgm {
    Config& cfg() {
        static Config s_config;
        return s_config;
    }

    void loadConfig() {
        auto* mod = Mod::get();
        auto& c = cfg();
        c.noclip       = mod->getSavedValue<bool>("noclip", false);
        c.autoSafeMode = mod->getSavedValue<bool>("auto-safe-mode", true);
        c.showHud      = mod->getSavedValue<bool>("show-hud", true);
        c.speed        = std::clamp(static_cast<float>(mod->getSavedValue<double>("speed", 1.0)), 0.1f, 3.f);
        c.slot         = std::clamp(mod->getSavedValue<int>("slot", 1), 1, 5);
    }

    void saveConfig() {
        auto* mod = Mod::get();
        auto const& c = cfg();
        mod->setSavedValue<bool>("noclip", c.noclip);
        mod->setSavedValue<bool>("auto-safe-mode", c.autoSafeMode);
        mod->setSavedValue<bool>("show-hud", c.showHud);
        mod->setSavedValue<double>("speed", static_cast<double>(c.speed));
        mod->setSavedValue<int>("slot", c.slot);
    }
}

// Muat pengaturan setelah data simpanan mod selesai dibaca
$on_mod(DataLoaded) {
    tgm::loadConfig();
}

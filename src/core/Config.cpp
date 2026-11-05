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

        c.noclip         = mod->getSavedValue<bool>("noclip", false);
        c.noclipP1       = mod->getSavedValue<bool>("noclip-p1", true);
        c.noclipP2       = mod->getSavedValue<bool>("noclip-p2", true);
        c.batasTabrakan  = std::clamp(mod->getSavedValue<int>("batas-tabrakan", 0), 0, 100);
        c.autoSafeMode   = mod->getSavedValue<bool>("auto-safe-mode", true);
        c.noDeathEffect  = mod->getSavedValue<bool>("no-death-effect", false);
        c.autoCheckpoint = mod->getSavedValue<bool>("auto-checkpoint", false);
        c.checkpointDtk  = std::clamp(mod->getSavedValue<int>("checkpoint-dtk", 5), 1, 30);
        c.speed          = std::clamp(static_cast<float>(mod->getSavedValue<double>("speed", 1.0)), 0.1f, 3.f);

        c.showHud        = mod->getSavedValue<bool>("show-hud", true);
        c.showFps        = mod->getSavedValue<bool>("show-fps", true);
        c.showClicks     = mod->getSavedValue<bool>("show-clicks", true);
        c.showClock      = mod->getSavedValue<bool>("show-clock", false);
        c.showPercent    = mod->getSavedValue<bool>("show-percent", false);
        c.showAttempt    = mod->getSavedValue<bool>("show-attempt", false);
        c.showFrame      = mod->getSavedValue<bool>("show-frame", false);
        c.showTime       = mod->getSavedValue<bool>("show-time", false);
        c.showDeaths     = mod->getSavedValue<bool>("show-deaths", false);

        c.hudOpacity     = std::clamp(mod->getSavedValue<int>("hud-opacity", 70), 10, 100);
        c.hudScale       = std::clamp(mod->getSavedValue<int>("hud-scale", 100), 50, 150);
        c.hudCorner      = std::clamp(mod->getSavedValue<int>("hud-corner", 0), 0, 3);
        c.hudColor       = std::clamp(mod->getSavedValue<int>("hud-color", 0), 0, 4);

        c.slot           = std::clamp(mod->getSavedValue<int>("slot", 1), 1, 5);
        c.autoSaveMacro  = mod->getSavedValue<bool>("auto-save-macro", false);
        c.autoPlay       = mod->getSavedValue<bool>("auto-play", false);
    }

    void saveConfig() {
        auto* mod = Mod::get();
        auto const& c = cfg();

        mod->setSavedValue<bool>("noclip", c.noclip);
        mod->setSavedValue<bool>("noclip-p1", c.noclipP1);
        mod->setSavedValue<bool>("noclip-p2", c.noclipP2);
        mod->setSavedValue<int>("batas-tabrakan", c.batasTabrakan);
        mod->setSavedValue<bool>("auto-safe-mode", c.autoSafeMode);
        mod->setSavedValue<bool>("no-death-effect", c.noDeathEffect);
        mod->setSavedValue<bool>("auto-checkpoint", c.autoCheckpoint);
        mod->setSavedValue<int>("checkpoint-dtk", c.checkpointDtk);
        mod->setSavedValue<double>("speed", static_cast<double>(c.speed));

        mod->setSavedValue<bool>("show-hud", c.showHud);
        mod->setSavedValue<bool>("show-fps", c.showFps);
        mod->setSavedValue<bool>("show-clicks", c.showClicks);
        mod->setSavedValue<bool>("show-clock", c.showClock);
        mod->setSavedValue<bool>("show-percent", c.showPercent);
        mod->setSavedValue<bool>("show-attempt", c.showAttempt);
        mod->setSavedValue<bool>("show-frame", c.showFrame);
        mod->setSavedValue<bool>("show-time", c.showTime);
        mod->setSavedValue<bool>("show-deaths", c.showDeaths);

        mod->setSavedValue<int>("hud-opacity", c.hudOpacity);
        mod->setSavedValue<int>("hud-scale", c.hudScale);
        mod->setSavedValue<int>("hud-corner", c.hudCorner);
        mod->setSavedValue<int>("hud-color", c.hudColor);

        mod->setSavedValue<int>("slot", c.slot);
        mod->setSavedValue<bool>("auto-save-macro", c.autoSaveMacro);
        mod->setSavedValue<bool>("auto-play", c.autoPlay);
    }

    void resetConfig() {
        cfg() = Config{};
        saveConfig();
    }
}

// Muat pengaturan setelah data simpanan mod selesai dibaca
$on_mod(DataLoaded) {
    tgm::loadConfig();
}

#pragma once
// Penentu apakah saat ini ada cheat yang aktif (dipakai Mode Aman, HUD, dan titik Cheat Indicator).

#include <Geode/Geode.hpp>

#include "Config.hpp"
#include "../bot/Bot.hpp"
#include <cmath>
#include <string>
#include <vector>

namespace tgm {
    // Hitbox di mode normal dihitung cheat (di practice mode GD memang menyediakannya).
    // Sengaja tidak memeriksa m_isTestMode: Mode Aman memakai testmode, sehingga akan saling memicu.
    inline bool hitboxMenipu() {
        if (!cfg().hitbox) return false;
        auto* pl = ::PlayLayer::get();
        return pl && !pl->m_isPracticeMode;
    }

    inline bool cheatsActive() {
        auto const& c = cfg();
        return c.noclip
            || c.lompatBebas
            || c.turbo
            || c.frameStepper
            || c.kebalPaku
            || std::abs(c.speed - 1.f) > 0.001f
            || bot::mode() == bot::Mode::Putar
            || hitboxMenipu();
    }

    // Nama cheat yang sedang aktif (untuk info di menu)
    inline std::vector<std::string> daftarCheat() {
        auto const& c = cfg();
        std::vector<std::string> d;
        if (c.noclip) d.push_back("Noclip");
        if (c.lompatBebas) d.push_back("Jump Hack");
        if (c.turbo) d.push_back("Turbo");
        if (c.kebalPaku) d.push_back("Kebal Paku");
        if (c.frameStepper) d.push_back("Frame Stepper");
        if (std::abs(c.speed - 1.f) > 0.001f) d.push_back("Speedhack");
        if (bot::mode() == bot::Mode::Putar) d.push_back("Bot");
        if (hitboxMenipu()) d.push_back("Hitbox");
        return d;
    }

    // Progres level harus dicegah tersimpan: Mode Aman manual, atau otomatis saat ada cheat aktif
    inline bool perluModeAman() {
        auto const& c = cfg();
        return c.safeMode || (c.autoSafeMode && cheatsActive());
    }
}

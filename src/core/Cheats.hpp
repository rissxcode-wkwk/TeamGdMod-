#pragma once
// Penentu apakah saat ini ada cheat yang aktif (dipakai Mode Aman & HUD).

#include "Config.hpp"
#include "../bot/Bot.hpp"
#include <cmath>

namespace tgm {
    inline bool cheatsActive() {
        auto const& c = cfg();
        return c.noclip
            || c.lompatBebas
            || c.turbo
            || c.frameStepper
            || std::abs(c.speed - 1.f) > 0.001f
            || bot::mode() == bot::Mode::Putar;
    }

    // Progres level harus dicegah tersimpan: Mode Aman manual, atau otomatis saat ada cheat aktif
    inline bool perluModeAman() {
        auto const& c = cfg();
        return c.safeMode || (c.autoSafeMode && cheatsActive());
    }
}

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
            || std::abs(c.speed - 1.f) > 0.001f
            || bot::mode() == bot::Mode::Putar;
    }
}

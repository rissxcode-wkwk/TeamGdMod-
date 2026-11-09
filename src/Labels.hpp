#pragma once
#include "Features.hpp"
#include <chrono>
#include <ctime>
#include <deque>

namespace tg::lab {

inline int noclipDeaths = 0;
inline long frames = 0, hitFrames = 0;
inline bool hitThisFrame = false, prevHit = false;
inline float fps = 60.f;
inline std::deque<double> clicks;

inline double nowSec() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void click() { clicks.push_back(nowSec()); if (clicks.size() > 400) clicks.pop_front(); }
inline int cps() {
    double t = nowSec();
    while (!clicks.empty() && t - clicks.front() > 1.0) clicks.pop_front();
    return (int)clicks.size();
}
inline void resetRun() { noclipDeaths = 0; frames = 0; hitFrames = 0; hitThisFrame = false; prevHit = false; }
inline void tickHit() {
    if (hitThisFrame) { ++hitFrames; if (!prevHit) ++noclipDeaths; }
    prevHit = hitThisFrame;
    hitThisFrame = false;
}

template <class P> constexpr bool hasAttempts = requires(P* p) { (int)p->m_attempts; };
template <class P> constexpr bool hasLevelTime = requires(P* p) { (double)p->m_gameState.m_levelTime; };
template <class P> constexpr bool hasHitboxes =
    requires(P* p) { p->m_isDebugDrawEnabled = true; } || requires(P* p) { p->m_debugDrawNode->setVisible(true); };
template <class E> constexpr bool hasPitch = requires(E* e, float v) { e->m_globalChannel->setPitch(v); };

template <class L> void forceHitboxes(L* l) {
    if constexpr (requires { l->m_isDebugDrawEnabled = true; }) l->m_isDebugDrawEnabled = true;
    if constexpr (requires { l->m_debugDrawNode->setVisible(true); }) l->m_debugDrawNode->setVisible(true);
}
template <class P> constexpr bool hasTrail = requires(P* p) { p->m_regularTrail->setVisible(false); };
template <class P> constexpr bool hasRobotFire = requires(P* p) { p->m_robotFire->setVisible(false); };
template <class P> constexpr bool hasGhost = requires(P* p) { p->m_ghostTrail->setVisible(false); };
template <class P> constexpr bool hasSolid = requires(P* p) { p->m_waveTrail->m_isSolid = true; };
template <class P> constexpr bool hasGround = requires(P* p) { p->m_isOnGround = true; };
template <class P> constexpr bool hasVehParts = requires(P* p) { p->m_vehicleGroundParticles->setVisible(false); }
    || requires(P* p) { p->m_playerGroundParticles->setVisible(false); }
    || requires(P* p) { p->m_trailingParticles->setVisible(false); };
template <class P> constexpr bool hasPctLabel = requires(P* p) { p->m_percentageLabel->setString("x"); };

template <class P> void visualTweaks(P* p) {
    auto doPlayer = [&](auto* pl) {
        if (!pl) return;
        if (cache().noTrail) { if constexpr (requires { pl->m_regularTrail->setVisible(false); }) pl->m_regularTrail->setVisible(false); }
        if (cache().noRobot) { if constexpr (requires { pl->m_robotFire->setVisible(false); }) pl->m_robotFire->setVisible(false); }
        if (cache().noGhost) { if constexpr (requires { pl->m_ghostTrail->setVisible(false); }) pl->m_ghostTrail->setVisible(false); }
        if (cache().solidWave) { if constexpr (requires { pl->m_waveTrail->m_isSolid = true; }) pl->m_waveTrail->m_isSolid = true; }
        if (cache().jumpHack) { if constexpr (requires { pl->m_isOnGround = true; }) pl->m_isOnGround = true; }
        if (cache().noVehPart) {
#define TG_HIDE(m) if constexpr (requires { pl->m->setVisible(false); }) pl->m->setVisible(false);
            TG_HIDE(m_vehicleGroundParticles) TG_HIDE(m_playerGroundParticles) TG_HIDE(m_trailingParticles)
            TG_HIDE(m_shipClickParticles) TG_HIDE(m_ufoClickParticles) TG_HIDE(m_robotBurstParticles)
            TG_HIDE(m_dashParticles) TG_HIDE(m_swingBurstParticles1) TG_HIDE(m_swingBurstParticles2)
#undef TG_HIDE
        }
    };
    doPlayer(p->m_player1);
    doPlayer(p->m_player2);
}
template <class P> void accuratePercent(P* p) {
    if constexpr (requires { p->m_percentageLabel->setString("x"); })
        if (p->m_percentageLabel && p->m_percentageLabel->isVisible())
            p->m_percentageLabel->setString(fmt::format("{:.2f}%", p->getCurrentPercent()).c_str());
}
template <class E> void setPitch(E* e, float v) {
    if constexpr (requires { e->m_globalChannel->setPitch(v); }) e->m_globalChannel->setPitch(v);
}

// akses member lewat template + requires agar tetap compile bila nama field berubah antar versi GD
template <class P> int attempts(P* p) {
    if constexpr (requires { (int)p->m_attempts; }) return (int)p->m_attempts; else return 0;
}
template <class P> double levelTime(P* p) {
    if constexpr (requires { (double)p->m_gameState.m_levelTime; }) return (double)p->m_gameState.m_levelTime;
    else return 0.0;
}
template <class P> int bestPercent(P* p) {
    if (!p->m_level) return 0;
    auto& lv = *p->m_level;
    if constexpr (requires { (int)lv.m_normalPercent.value(); }) return (int)lv.m_normalPercent.value();
    else if constexpr (requires { (int)lv.m_normalPercent; }) return (int)lv.m_normalPercent;
    else return 0;
}
template <class P> bool testMode(P* p) {
    if constexpr (requires { (bool)p->m_isTestMode; }) return (bool)p->m_isTestMode; else return false;
}

inline std::vector<std::string> customLabels() {
    return splitList(Mod::get()->getSavedValue<std::string>("custom-labels", ""), '|');
}

template <class P> std::string text(P* p) {
    std::string s;
    auto line = [&](std::string const& l) { s += l; s += "\n"; };
    if (on("labels-cheat-indicator")) line(cache().cheating ? "Cheats: ON" : "Cheats: OFF");
    if (on("labels-testmode") && testMode(p)) line("Testmode");
    if (on("labels-attempt")) line(fmt::format("Attempt {}", attempts(p)));
    if (on("labels-percentage")) line(fmt::format("{}%", (int)p->getCurrentPercent()));
    if (on("labels-level-time")) {
        double t = levelTime(p);
        line(fmt::format("{}:{:05.2f}", (int)(t / 60), std::fmod(t, 60.0)));
    }
    if (on("labels-best-run")) line(fmt::format("Best {}%", bestPercent(p)));
    if (on("labels-clock")) {
        std::time_t t = std::time(nullptr);
        char b[16];
        std::strftime(b, sizeof b, "%H:%M:%S", std::localtime(&t));
        line(b);
    }
    if (on("labels-fps")) line(fmt::format("{} FPS", (int)std::round(fps)));
    if (on("labels-cps")) line(fmt::format("{} CPS", cps()));
    if (on("labels-noclip-accuracy")) {
        double acc = frames > 0 ? 100.0 * (1.0 - (double)hitFrames / (double)frames) : 100.0;
        line(fmt::format("Accuracy {:.2f}%", acc));
    }
    if (on("labels-noclip-deaths")) line(fmt::format("Noclip deaths {}", noclipDeaths));
    for (auto& c : customLabels()) line(c);
    if (!s.empty()) s.pop_back();
    return s;
}

} // namespace tg::lab

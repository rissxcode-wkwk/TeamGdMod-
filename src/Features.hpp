#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
using namespace geode::prelude;

namespace tg {

enum class Kind { Toggle, Slider, Dropdown, Color, Action };

// ready=false -> tampil abu-abu & tidak bisa disentuh (belum diimplementasi; bukan toggle palsu)
struct Feature {
    std::string id, name, cat;
    Kind kind = Kind::Toggle;
    bool ready = false;
    float min = 0, max = 1, def = 0;
    std::vector<std::string> opts;
    unsigned col = 0xFFFFFF;
};

inline std::string slug(std::string s) {
    for (auto& c : s) c = (c == ' ' || c == '/') ? '-' : (char)std::tolower((unsigned char)c);
    return s;
}

inline std::vector<std::string> const& categories() {
    static std::vector<std::string> c = {
        "Global", "Level", "Bypass", "Player", "Bot", "Creator", "Internal Recorder",
        "Labels", "Shortcuts", "Keybinds", "Interface", "Config"};
    return c;
}

inline std::vector<std::string> const& themeNames() {
    static std::vector<std::string> v = {"Purple", "Cyan", "Crimson", "Emerald", "Amber", "Mono"};
    return v;
}
// {accent, background}
inline std::pair<unsigned, unsigned> themeColors(int i) {
    static std::pair<unsigned, unsigned> t[] = {
        {0x9628BE, 0x12121A}, {0x1AA5C8, 0x0E1519}, {0xC8283C, 0x1A1011},
        {0x23B16A, 0x0F1712}, {0xD79B1E, 0x18150E}, {0x8C8C8C, 0x141414}};
    return t[std::clamp(i, 0, 5)];
}

inline std::vector<Feature> buildFeatures() {
    std::vector<Feature> r;
    auto add = [&](char const* c, char const* n, Kind k, float mn = 0, float mx = 1, float df = 0,
                   std::vector<std::string> o = {}, unsigned col = 0xFFFFFF) {
        Feature f;
        f.id = slug(std::string(c) + "-" + n);
        f.name = n; f.cat = c; f.kind = k; f.min = mn; f.max = mx; f.def = df;
        f.opts = std::move(o); f.col = col;
        r.push_back(std::move(f));
    };
    auto T = [&](char const* c, char const* n, bool d = false) { add(c, n, Kind::Toggle, 0, 1, d ? 1.f : 0.f); };
    auto S = [&](char const* c, char const* n, float mn, float mx, float df) { add(c, n, Kind::Slider, mn, mx, df); };
    auto D = [&](char const* c, char const* n, std::vector<std::string> o, int df = 0) {
        add(c, n, Kind::Dropdown, 0, (float)o.size() - 1, (float)df, o);
    };
    auto C = [&](char const* c, char const* n, unsigned col) { add(c, n, Kind::Color, 0, 1, 0, {}, col); };
    auto A = [&](char const* c, char const* n) { add(c, n, Kind::Action); };
    auto TL = [&](char const* c, std::initializer_list<char const*> l) { for (auto n : l) T(c, n); };

    // GLOBAL
    S("Global", "FPS", 30, 360, 60);
    T("Global", "FPS Bypass");
    S("Global", "TPS", 60, 960, 240);
    T("Global", "Physics Bypass");
    S("Global", "Speedhack", 0.1f, 3.f, 1.f);
    S("Global", "Pitch Shift", 0.5f, 2.f, 1.f);
    S("Global", "Audio Speed", 0.5f, 2.f, 1.f);
    TL("Global", {"Sync with Speedhack", "Auto Safe Mode", "Compact Editor Levels",
                  "Compact Profile Comments", "Discord RPC", "Lock Cursor", "No Short Numbers", "Safe Mode"});
    S("Global", "Transition Speed", 0.f, 1.f, 0.5f);
    T("Global", "Transparent Lists");

    // LEVEL
    TL("Level", {"Practice Completion", "Accurate Percentage", "All Modes in Platformer", "Always Show Coins",
        "Auto LDM", "Auto Pickup Coins", "Auto Practice Mode", "Auto Song Download", "Force Platformer",
        "Freeze Attempts", "Hide Level Complete VFX", "Hide Pause Menu"});
    S("Level", "Hitbox Multiplier", 0.5f, 3.f, 1.f);
    TL("Level", {"Instant Mirror", "Legacy Reverse Physics", "Match Percent/Time Fonts", "Mute Level SFX",
        "No Mirror", "No Object Glow", "No Particles", "No Portal Lightning", "No Shaders", "Pause Countdown",
        "Pause During Complete", "Practice Coins", "Practice Fix", "Show Hitboxes", "Show Triggers",
        "Smart StartPos", "StartPos Switcher"});

    // BYPASS
    TL("Bypass", {"Allow Low Volume", "Character Filter Bypass", "Character Limit Bypass", "Checkpoint Limit",
        "Comment History Bypass", "Instant Complete", "Menu Gameplay", "Music Customizer", "Pause Buffering",
        "Practice Music Sync", "Unlock Icons", "Unlock Main Levels", "Unlock Shops", "Unlock Vaults"});

    // PLAYER
    TL("Player", {"Always Show Trail", "AutoClicker"});
    S("Player", "AutoClicker Interval", 2, 60, 6);
    TL("Player", {"Auto Kill", "Click Teleport", "Custom Wave Trail", "Frame Stepper", "Jump Hack",
        "Mute Level Audio on Death", "Mute Rewards SFX on Death", "No Death Effect", "No Ghost Trail",
        "No Hide Trigger", "No Platformer Jump Animation", "No Respawn Flash", "No Robot Fire",
        "No Spider Dash", "No Trail", "No Vehicle Particles", "Noclip", "Show Trajectory", "Solid Wave Trail"});
    S("Player", "Auto Kill Percent", 1, 100, 50);
    S("Player", "Respawn Delay", 0.f, 5.f, 1.f);

    // BOT
    TL("Bot", {"Bot Enable", "Record", "Playback"});
    D("Bot", "Replay Selection", {"(kosong)"});
    for (auto n : {"Create New Replay", "Save Replay", "Load Replay", "Delete Replay", "Replay Management"})
        A("Bot", n);

    // CREATOR
    TL("Creator", {"Custom Object Bypass", "Default Song Bypass", "Free Scroll", "Hide Triggers", "Hide UI",
        "Level Copy Bypass", "Level Edit", "No Copyright Mark", "Reset Percent on Save", "Slider Limit",
        "Smooth Editor Trail", "Verify Bypass"});

    // INTERNAL RECORDER
    A("Internal Recorder", "Start Recording");
    A("Internal Recorder", "Stop Recording");
    S("Internal Recorder", "Framerate", 30, 240, 60);
    S("Internal Recorder", "Endscreen Duration", 0, 10, 3.4f);
    S("Internal Recorder", "Bitrate", 1, 200, 30);
    S("Internal Recorder", "Resolution X", 640, 3840, 1920);
    S("Internal Recorder", "Resolution Y", 360, 2160, 1080);
    D("Internal Recorder", "Codec Selection", {"libx264", "h264_nvenc", "h264_amf", "av1"});
    D("Internal Recorder", "Audio Mode", {"Always record", "Record only level", "Off"});
    A("Internal Recorder", "Colorspace Arguments");
    D("Internal Recorder", "Presets", {"CPU", "NVIDIA", "AMD"});
    A("Internal Recorder", "CPU Preset");
    A("Internal Recorder", "NVIDIA Preset");
    A("Internal Recorder", "AMD Preset");

    // LABELS
    TL("Labels", {"Show Labels", "Cheat Indicator", "Testmode", "Attempt", "Percentage", "Level Time",
                  "Best Run", "Clock", "FPS", "CPS", "Noclip Accuracy", "Noclip Deaths"});
    S("Labels", "Labels Scale", 0.4f, 1.5f, 0.8f);
    S("Labels", "Labels Opacity", 0.2f, 1.f, 0.8f);
    A("Labels", "Add New Label");
    A("Labels", "Clear Custom Labels");

    // SHORTCUTS (aksi langsung; tombol keyboard-nya ada di Pengaturan mod Geode)
    for (auto n : {"P1 Jump", "P2 Jump", "Show Options", "Uncomplete Level", "Restart Level",
                   "Toggle Practice Mode", "Place Checkpoint", "Remove Checkpoint", "Step Frame"})
        A("Shortcuts", n);

    // KEYBINDS
    A("Keybinds", "Open Menu");
    T("Keybinds", "In-Game Only");
    A("Keybinds", "Rebind Key");
    A("Keybinds", "Clear Keybind");
    A("Keybinds", "Reset Default");

    // INTERFACE
    D("Interface", "Theme", themeNames(), 0);
    S("Interface", "UI Scale", 0.5f, 1.4f, 1.f);
    D("Interface", "Font", {"bigFont", "goldFont", "chatFont", "gjFont01", "gjFont02"}, 0);
    S("Interface", "Font Size", 0.6f, 1.6f, 1.f);
    A("Interface", "Reload Fonts");
    D("Interface", "Layout Type", {"Panels", "Tabbed"}, 0);
    D("Interface", "Style", {"Rounded", "Flat", "Outline"}, 0);
    T("Interface", "Enable Blur");
    C("Interface", "Accent Color", 0x9628BE);
    C("Interface", "Background Color", 0x12121A);
    S("Interface", "Panel Opacity", 0.3f, 1.f, 0.92f);
    S("Interface", "Border Width", 0.f, 4.f, 1.f);
    S("Interface", "Corner Radius", 0.f, 12.f, 5.f);
    T("Interface", "Animations", true);
    S("Interface", "Animation Speed", 0.3f, 3.f, 1.f);

    // CONFIG
    T("Config", "Auto Save", true);
    A("Config", "Save Configuration");
    A("Config", "Load Configuration");
    A("Config", "Reset Configuration");

    static const std::set<std::string> ready = {
        "global-fps", "global-fps-bypass", "global-speedhack", "global-pitch-shift", "global-audio-speed",
        "global-sync-with-speedhack", "global-auto-safe-mode", "global-safe-mode",
        "level-auto-practice-mode", "level-freeze-attempts", "level-show-hitboxes", "level-no-mirror",
        "level-hide-pause-menu", "level-accurate-percentage", "player-no-robot-fire", "player-no-trail",
        "level-auto-ldm", "player-auto-kill", "player-auto-kill-percent",
        "bypass-unlock-shops", "bypass-unlock-vaults", "player-jump-hack",
        "player-no-ghost-trail", "player-no-vehicle-particles", "player-solid-wave-trail",
        "bypass-unlock-icons",
        "player-autoclicker", "player-autoclicker-interval", "player-frame-stepper",
        "player-no-death-effect", "player-noclip",
        "bot-bot-enable", "bot-record", "bot-playback", "bot-replay-selection", "bot-create-new-replay",
        "bot-save-replay", "bot-load-replay", "bot-delete-replay", "bot-replay-management",
        "labels-show-labels", "labels-cheat-indicator", "labels-testmode", "labels-attempt",
        "labels-percentage", "labels-level-time", "labels-best-run", "labels-clock", "labels-fps",
        "labels-cps", "labels-noclip-accuracy", "labels-noclip-deaths", "labels-labels-scale",
        "labels-labels-opacity", "labels-add-new-label", "labels-clear-custom-labels",
        "shortcuts-p1-jump", "shortcuts-p2-jump", "shortcuts-show-options", "shortcuts-restart-level",
        "shortcuts-toggle-practice-mode", "shortcuts-place-checkpoint", "shortcuts-remove-checkpoint",
        "shortcuts-step-frame",
        "keybinds-open-menu", "keybinds-in-game-only", "keybinds-rebind-key", "keybinds-clear-keybind",
        "keybinds-reset-default",
        "interface-theme", "interface-ui-scale", "interface-font", "interface-font-size",
        "interface-reload-fonts", "interface-layout-type", "interface-style", "interface-accent-color",
        "interface-background-color", "interface-panel-opacity", "interface-border-width",
        "interface-corner-radius", "interface-animations", "interface-animation-speed",
        "config-auto-save", "config-save-configuration", "config-load-configuration",
        "config-reset-configuration"};
    for (auto& f : r) f.ready = ready.count(f.id) > 0;
    return r;
}

inline std::vector<Feature> const& features() {
    static std::vector<Feature> v = buildFeatures();
    return v;
}
inline Feature const* find(std::string const& id) {
    for (auto& f : features()) if (f.id == id) return &f;
    return nullptr;
}

// ---------------------------------------------------------------- state
inline std::unordered_map<std::string, float>& vals() { static std::unordered_map<std::string, float> m; return m; }
inline std::string skey(std::string const& id) { return "v:" + id; }
inline float get(std::string const& id) { auto it = vals().find(id); return it == vals().end() ? 0.f : it->second; }
inline bool on(std::string const& id) { return get(id) > 0.5f; }

inline void setRaw(std::string const& id, float v) {
    vals()[id] = v;
    Mod::get()->setSavedValue<double>(skey(id), (double)v);
}
inline ccColor3B color(Feature const& f) {
    return {(GLubyte)get(f.id + "#0"), (GLubyte)get(f.id + "#1"), (GLubyte)get(f.id + "#2")};
}
inline ccColor3B color(char const* id) { auto f = find(id); return f ? color(*f) : ccColor3B{255, 255, 255}; }
inline void setColor(Feature const& f, ccColor3B c) {
    setRaw(f.id + "#0", c.r); setRaw(f.id + "#1", c.g); setRaw(f.id + "#2", c.b);
}

inline void loadAll() {
    for (auto& f : features()) {
        if (f.kind == Kind::Action) continue;
        if (f.kind == Kind::Color) {
            unsigned d[3] = {(f.col >> 16) & 255, (f.col >> 8) & 255, f.col & 255};
            for (int i = 0; i < 3; ++i) {
                auto k = f.id + "#" + std::to_string(i);
                vals()[k] = (float)Mod::get()->getSavedValue<double>(skey(k), (double)d[i]);
            }
        } else {
            vals()[f.id] = (float)Mod::get()->getSavedValue<double>(skey(f.id), (double)f.def);
        }
    }
}

// nilai panas yang dibaca tiap frame
struct Cache {
    bool noclip = false, unlock = false, hitboxes = false, autoPractice = false, freeze = false;
    bool autoKill = false;
    bool unlockShops = false, charLimit = false, jumpHack = false, noGhost = false, noVehPart = false, solidWave = false;
    bool noMirror = false, hidePause = false, accPct = false, noRobot = false, noTrail = false;
    bool noDeathFx = false, autoclick = false, stepper = false, botRec = false, botPlay = false;
    bool cheating = false;
    int clickInterval = 6;
    float speed = 1.f, pitch = 1.f;
    bool safe() const { return on("global-safe-mode") || (on("global-auto-safe-mode") && cheating); }
};
inline Cache& cache() { static Cache c; return c; }
inline bool g_menuOpen = false;
// diisi saat startup: fitur yang field/API-nya tidak ada di versi GD/SDK ini
inline std::set<std::string> g_unsupported;
inline bool g_stepPending = false;

inline void applyFPS() {
    if (!Mod::get()->hasSavedValue(skey("global-fps"))) return;
    float fps = std::max(30.f, get("global-fps"));
    if (!on("global-fps-bypass")) fps = std::min(fps, 240.f);
    CCApplication::sharedApplication()->setAnimationInterval(1.0 / fps);
}

inline void refresh() {
    auto& c = cache();
    c.noclip = on("player-noclip");
    c.unlock = on("bypass-unlock-icons");
    c.hitboxes = on("level-show-hitboxes");
    c.autoPractice = on("level-auto-practice-mode");
    c.freeze = on("level-freeze-attempts");
    c.noDeathFx = on("player-no-death-effect");
    c.unlockShops = on("bypass-unlock-shops") || on("bypass-unlock-vaults");
    c.charLimit = on("bypass-character-limit-bypass");
    c.autoKill = on("player-auto-kill");
    c.jumpHack = on("player-jump-hack");
    c.noGhost = on("player-no-ghost-trail");
    c.noVehPart = on("player-no-vehicle-particles");
    c.solidWave = on("player-solid-wave-trail");
    c.noMirror = on("level-no-mirror");
    c.hidePause = on("level-hide-pause-menu");
    c.accPct = on("level-accurate-percentage");
    c.noRobot = on("player-no-robot-fire");
    c.noTrail = on("player-no-trail");
    c.autoclick = on("player-autoclicker");
    c.stepper = on("player-frame-stepper");
    c.clickInterval = std::max(2, (int)std::round(get("player-autoclicker-interval")));
    c.speed = std::clamp(get("global-speedhack"), 0.1f, 3.f);
    float p = get("global-pitch-shift") * get("global-audio-speed");
    if (on("global-sync-with-speedhack")) p *= c.speed;
    c.pitch = std::clamp(p, 0.1f, 4.f);
    c.botRec = on("bot-bot-enable") && on("bot-record");
    c.botPlay = on("bot-bot-enable") && on("bot-playback") && !c.botRec;
    c.cheating = c.noclip || c.jumpHack || c.autoclick || c.stepper || c.botRec || c.botPlay ||
                 std::abs(c.speed - 1.f) > 0.001f;
    applyFPS();
}
inline void setB(Feature const& f, bool v) { setRaw(f.id, v ? 1.f : 0.f); refresh(); }
inline void setV(Feature const& f, float v) {
    v = std::clamp(v, f.min, f.max);
    if (f.kind == Kind::Dropdown) v = std::round(v);
    setRaw(f.id, v);
    refresh();
}

// ---------------------------------------------------------------- config file
// getSaveDir() bertipe berbeda antar versi SDK (std::filesystem::path / asp::fs::Path) -> ubah ke string dulu
template <class P> std::string pstr(P const& p) {
    if constexpr (requires { std::string(p.string()); }) return std::string(p.string());
    else if constexpr (requires { std::string(p.str()); }) return std::string(p.str());
    else if constexpr (requires { std::string(p.native()); }) return std::string(p.native());
    else return std::string(p);
}
inline std::filesystem::path saveDir() { return std::filesystem::path(pstr(Mod::get()->getSaveDir())); }
inline std::filesystem::path cfgPath() { return saveDir() / "config.txt"; }

inline bool saveConfig() {
    std::string s;
    for (auto& f : features()) {
        if (f.kind == Kind::Action) continue;
        if (f.kind == Kind::Color)
            s += fmt::format("{}={},{},{}\n", f.id, (int)get(f.id + "#0"), (int)get(f.id + "#1"), (int)get(f.id + "#2"));
        else
            s += fmt::format("{}={}\n", f.id, get(f.id));
    }
    return geode::utils::file::writeString(cfgPath().string(), s).isOk();
}
inline bool loadConfig() {
    auto r = geode::utils::file::readString(cfgPath().string());
    if (r.isErr()) return false;
    std::istringstream in(r.unwrap());
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto f = find(line.substr(0, eq));
        if (!f) continue;
        auto v = line.substr(eq + 1);
        try {
            if (f->kind == Kind::Color) {
                int r_, g_, b_;
                if (sscanf(v.c_str(), "%d,%d,%d", &r_, &g_, &b_) == 3)
                    setColor(*f, {(GLubyte)std::clamp(r_, 0, 255), (GLubyte)std::clamp(g_, 0, 255), (GLubyte)std::clamp(b_, 0, 255)});
            } else if (f->kind == Kind::Toggle) {
                setRaw(f->id, std::stof(v) > 0.5f ? 1.f : 0.f);
            } else {
                setRaw(f->id, std::clamp(std::stof(v), f->min, f->max));
            }
        } catch (...) {}
    }
    refresh();
    return true;
}
inline void resetConfig() {
    for (auto& f : features()) {
        if (f.kind == Kind::Action) continue;
        if (f.kind == Kind::Color) {
            setColor(f, {(GLubyte)((f.col >> 16) & 255), (GLubyte)((f.col >> 8) & 255), (GLubyte)(f.col & 255)});
        } else {
            setRaw(f.id, f.def);
        }
    }
    refresh();
}

// ---------------------------------------------------------------- string list helpers (favorit/riwayat)
inline std::vector<std::string> splitList(std::string const& s, char sep = ',') {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) { if (c == sep) { if (!cur.empty()) out.push_back(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
inline std::string joinList(std::vector<std::string> const& v, char sep = ',') {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) { if (i) s += sep; s += v[i]; }
    return s;
}
inline std::vector<std::string> favorites() { return splitList(Mod::get()->getSavedValue<std::string>("favorites", "")); }
inline bool isFavorite(std::string const& id) { auto f = favorites(); return std::find(f.begin(), f.end(), id) != f.end(); }
inline void toggleFavorite(std::string const& id) {
    auto f = favorites();
    auto it = std::find(f.begin(), f.end(), id);
    if (it != f.end()) f.erase(it); else f.push_back(id);
    Mod::get()->setSavedValue<std::string>("favorites", joinList(f));
}
inline std::vector<std::string> history() { return splitList(Mod::get()->getSavedValue<std::string>("history", "")); }
inline void pushHistory(std::string const& id) {
    auto h = history();
    h.erase(std::remove(h.begin(), h.end(), id), h.end());
    h.insert(h.begin(), id);
    if (h.size() > 12) h.resize(12);
    Mod::get()->setSavedValue<std::string>("history", joinList(h));
}

} // namespace tg

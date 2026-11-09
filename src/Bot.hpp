#pragma once
#include "Features.hpp"
#include <filesystem>

// Bot sederhana berbasis frame: merekam klik P1/P2 lalu memutarnya ulang.
// Catatan: akurasi playback bergantung pada FPS/physics yang konsisten.
namespace tg::bot {

struct Ev { int frame; bool down; bool p1; };
inline std::vector<Ev> rec;
inline int frame = 0;
inline size_t idx = 0;

inline std::filesystem::path dir() {
    auto d = Mod::get()->getSaveDir() / "replays";
    std::error_code ec;
    std::filesystem::create_directories(d, ec);
    return d;
}
inline std::string clean(std::string s) {
    std::string o;
    for (char c : s) if (std::isalnum((unsigned char)c) || c == '_' || c == '-') o += c;
    return o.substr(0, 32);
}
inline std::vector<std::string> list() {
    std::vector<std::string> out;
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(dir(), ec))
        if (e.path().extension() == ".tgr") out.push_back(e.path().stem().string());
    std::sort(out.begin(), out.end());
    return out;
}
inline std::string selected() {
    auto l = list();
    int i = (int)get("bot-replay-selection");
    return (i >= 0 && i < (int)l.size()) ? l[i] : "";
}
inline bool save(std::string name) {
    name = clean(name);
    if (name.empty()) return false;
    std::string s;
    for (auto& e : rec) s += fmt::format("{} {} {}\n", e.frame, e.down ? 1 : 0, e.p1 ? 1 : 0);
    return geode::utils::file::writeString(dir() / (name + ".tgr"), s).isOk();
}
inline bool load(std::string const& name) {
    if (name.empty()) return false;
    auto r = geode::utils::file::readString(dir() / (name + ".tgr"));
    if (r.isErr()) return false;
    rec.clear();
    std::istringstream in(r.unwrap());
    int f, d, p;
    while (in >> f >> d >> p) rec.push_back({f, d != 0, p != 0});
    std::sort(rec.begin(), rec.end(), [](Ev const& a, Ev const& b) { return a.frame < b.frame; });
    idx = 0;
    return true;
}
inline bool remove(std::string const& name) {
    std::error_code ec;
    return !name.empty() && std::filesystem::remove(dir() / (name + ".tgr"), ec);
}
inline bool rename(std::string const& from, std::string to) {
    to = clean(to);
    if (from.empty() || to.empty()) return false;
    std::error_code ec;
    std::filesystem::rename(dir() / (from + ".tgr"), dir() / (to + ".tgr"), ec);
    return !ec;
}

} // namespace tg::bot

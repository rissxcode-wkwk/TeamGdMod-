#pragma once

// Lapisan kompatibilitas Geode v5 -> v6 (GD 2.2091).
// Geode v6 tidak lagi memakai std::filesystem::path, semua API direktori
// sekarang mengembalikan asp::fs::Path. Header ini menjembatani keduanya
// supaya kode lama yang memakai std::filesystem tetap berfungsi.

#include <filesystem>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Dirs.hpp>
#include <asp/fs/std.hpp>

namespace eclipse::compat {

    /// @brief Ubah asp::fs::Path / PathView menjadi std::filesystem::path.
    template <typename T>
    inline std::filesystem::path toStd(T const& path) {
        return path.toStdPath().unwrapOr(std::filesystem::path{});
    }

    /// @brief Ubah std::filesystem::path menjadi asp::fs::Path (untuk API Geode v6).
    inline asp::fs::Path toAsp(std::filesystem::path const& path) {
        return asp::fs::Path::fromStdPath(path);
    }

    inline std::filesystem::path saveDir() { return toStd(geode::Mod::get()->getSaveDir()); }
    inline std::filesystem::path resourcesDir() { return toStd(geode::Mod::get()->getResourcesDir()); }
    inline std::filesystem::path configDir() { return toStd(geode::Mod::get()->getConfigDir()); }
    inline std::filesystem::path gameDir() { return toStd(geode::dirs::getGameDir()); }
    inline std::filesystem::path gameSaveDir() { return toStd(geode::dirs::getSaveDir()); }

}

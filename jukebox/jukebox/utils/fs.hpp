#pragma once

// Filesystem helpers for Jukebox.
//
// Geode v6 replaced std::filesystem::path with asp::fs::Path in all of its APIs
// (Mod::getSaveDir, utils::file::*, file::pick, ...). Jukebox keeps using
// std::filesystem::path internally and converts only at the Geode boundary
// using the bridge documented in the v6 migration guide
// (<asp/fs/std.hpp>: Path::fromStdPath / Path::toStdPath).
//
// All helpers here are non-throwing.

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <Geode/Result.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/file.hpp>
#include <asp/fs/std.hpp>
#include <matjson.hpp>

namespace jukebox::fs {

using Path = std::filesystem::path;

// ---- string <-> path (always UTF-8) ----

[[nodiscard]] inline std::string toString(const Path& path) {
    const auto u8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

[[nodiscard]] inline Path fromString(std::string_view str) {
    return Path(std::u8string(reinterpret_cast<const char8_t*>(str.data()), str.size()));
}

// Extension WITH the leading dot (std::filesystem semantics), as a UTF-8 string.
// asp::fs::Path::extension() returns it WITHOUT the dot, so we never use that one.
[[nodiscard]] inline std::string extension(const Path& path) { return toString(path.extension()); }
[[nodiscard]] inline std::string filename(const Path& path) { return toString(path.filename()); }
[[nodiscard]] inline std::string stem(const Path& path) { return toString(path.stem()); }

// ---- std <-> asp bridge (Geode v6 boundary) ----

[[nodiscard]] inline Path toStd(const Path& path) { return path; }
[[nodiscard]] inline Path toStd(const asp::fs::Path& path) { return Path(path.native()); }

[[nodiscard]] inline asp::fs::Path toAsp(const Path& path) { return asp::fs::Path::fromStdPath(path); }

// ---- queries ----

[[nodiscard]] inline bool exists(const Path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec;
}

[[nodiscard]] inline bool isDirectory(const Path& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && !ec;
}

[[nodiscard]] inline std::optional<std::uintmax_t> fileSize(const Path& path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        return std::nullopt;
    }
    return size;
}

// ---- mutations ----

[[nodiscard]] inline geode::Result<> createDirs(const Path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        return geode::Err("{}", ec.message());
    }
    return geode::Ok();
}

[[nodiscard]] inline geode::Result<> remove(const Path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) {
        return geode::Err("{}", ec.message());
    }
    return geode::Ok();
}

[[nodiscard]] inline geode::Result<> removeAll(const Path& path) {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    if (ec) {
        return geode::Err("{}", ec.message());
    }
    return geode::Ok();
}

[[nodiscard]] inline geode::Result<> rename(const Path& from, const Path& to) {
    std::error_code ec;
    std::filesystem::rename(from, to, ec);
    if (ec) {
        return geode::Err("{}", ec.message());
    }
    return geode::Ok();
}

[[nodiscard]] inline geode::Result<> copyFile(const Path& from, const Path& to, bool overwrite = false) {
    std::error_code ec;
    std::filesystem::copy_file(
        from, to, overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none,
        ec);
    if (ec) {
        return geode::Err("{}", ec.message());
    }
    return geode::Ok();
}

// Non-recursive listing of a directory's entries.
[[nodiscard]] inline geode::Result<std::vector<Path>> listDir(const Path& dir) {
    std::error_code ec;
    std::filesystem::directory_iterator it(dir, ec);
    if (ec) {
        return geode::Err("{}", ec.message());
    }

    std::vector<Path> out;
    for (const std::filesystem::directory_iterator end; it != end; it.increment(ec)) {
        if (ec) {
            return geode::Err("{}", ec.message());
        }
        out.push_back(it->path());
    }
    return geode::Ok(std::move(out));
}

// ---- Geode-facing file IO (takes/returns std paths, talks asp::fs::Path to Geode) ----

[[nodiscard]] inline Path saveDir() { return toStd(geode::Mod::get()->getSaveDir()); }

[[nodiscard]] inline geode::Result<matjson::Value> readJson(const Path& path) {
    return geode::utils::file::readJson(toAsp(path));
}

[[nodiscard]] inline geode::Result<> writeString(const Path& path, const std::string& data) {
    return geode::utils::file::writeString(toAsp(path), data);
}

template <class Bytes>
[[nodiscard]] inline geode::Result<> writeBinary(const Path& path, const Bytes& data) {
    return geode::utils::file::writeBinary(toAsp(path), data);
}

}  // namespace jukebox::fs

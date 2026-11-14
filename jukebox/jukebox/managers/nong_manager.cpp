#include <vector>
#include <jukebox/managers/nong_manager.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>

#include <Geode/cocos/platform/CCFileUtils.h>
#include <fmt/format.h>
#include <Geode/Result.hpp>
#include <Geode/binding/LevelTools.hpp>
#include <Geode/binding/MusicDownloadManager.hpp>
#include <jukebox/utils/gd_compat.hpp>
#include <Geode/binding/SongInfoObject.hpp>
#include <Geode/loader/Log.hpp>
#include <Geode/utils/file.hpp>
#include <jukebox/utils/fs.hpp>
#include <asp/iter.hpp>
#include <matjson.hpp>

#include <jukebox/compat/compat.hpp>
#include <jukebox/compat/v2.hpp>
#include <jukebox/events/get_song_info.hpp>
#include <jukebox/events/song_download_finished.hpp>
#include <jukebox/events/song_error.hpp>
#include <jukebox/managers/index_manager.hpp>
#include <jukebox/nong/nong.hpp>
#include <jukebox/nong/nong_serialize.hpp>
#include <jukebox/utils/random_string.hpp>

using namespace geode::prelude;

namespace jukebox {

std::optional<Nongs*> NongManager::getNongs(int songID) {
    if (!m_manifest.m_nongs.contains(songID)) {
        return std::nullopt;
    }

    return m_manifest.m_nongs[songID].get();
}

std::vector<std::string> NongManager::getVerifiedNongsForLevel(int levelID, std::vector<int> songIDs) {
    std::vector<std::string> verifiedNongs;

    for (const int songID : songIDs) {
        auto nongs = this->getNongs(songID);

        if (!nongs.has_value()) {
            continue;
        }

        // For each indexSong, check if it contains the given levelID in its
        // verifiedLevelIDs field.
        for (const auto indexSong : nongs.value()->indexSongs()) {
            if (auto& ids = indexSong->verifiedLevelIDs; std::ranges::find(ids, levelID) != ids.end()) {
                verifiedNongs.push_back(indexSong->uniqueID);
            }
        }
    }

    return verifiedNongs;
}

int NongManager::getCurrentManifestVersion() const { return m_manifest.m_version; }

int NongManager::getStoredIDCount() const { return m_manifest.m_nongs.size(); }

int NongManager::adjustSongID(int id, bool robtop) { return robtop ? (id < 0 ? id : -id - 1) : id; }

bool NongManager::hasSongID(int id) const { return m_manifest.m_nongs.contains(id); }

Result<Nongs*> NongManager::initSongID(SongInfoObject* obj, int id, bool robtop) {
    int adjusted = this->adjustSongID(id, robtop);

    if (this->hasSongID(adjusted)) {
        return Err("Song already exists");
    }

    if (!obj && robtop) {
        return Err("Critical. No song object for RobTop song");
    }

    if (obj && robtop) {
        std::string filename = jukebox::gd::getAudioFileName(id);
        std::filesystem::path gdDir = jukebox::fs::fromString(CCFileUtils::sharedFileUtils()->getWritablePath2());
        std::unique_ptr<Nongs> nongs = std::make_unique<Nongs>(Nongs{
            adjusted, LocalSong{SongMetadata{adjusted, jukebox::random_string(16), obj->m_songName, obj->m_artistName},
                                gdDir / "Resources" / filename}});

        Nongs* n = nongs.get();
        m_manifest.m_nongs.insert({adjusted, std::move(nongs)});
        IndexManager::get().registerIndexNongs(n);

        return Ok(n);
    }

    if (!obj) {
        // See if we already have it stored in the savefile
        obj = jukebox::gd::getSongInfoObject(id);
    }

    if (!obj) {
        // Try fetch song info from servers
        jukebox::gd::getSongInfo(id, true);
        std::unique_ptr<Nongs> nongs = std::make_unique<Nongs>(Nongs{id, LocalSong::createUnknown(id)});

        Nongs* n = nongs.get();
        m_manifest.m_nongs.insert({adjusted, std::move(nongs)});

        IndexManager::get().registerIndexNongs(n);
        return Ok(n);
    }

    // Finally, if obj exists just insert normally
    std::unique_ptr<Nongs> nongs = std::make_unique<Nongs>(Nongs{
        adjusted, LocalSong{SongMetadata{adjusted, jukebox::random_string(16), obj->m_songName, obj->m_artistName},
                            jukebox::fs::fromString(jukebox::gd::pathForSong(id))}});

    Nongs* n = nongs.get();
    m_manifest.m_nongs.insert({adjusted, std::move(nongs)});

    IndexManager::get().registerIndexNongs(n);
    return Ok(n);
}

bool NongManager::init() {
    if (m_initialized) {
        return true;
    }

    event::SongError().listen([](const event::SongErrorData& event) { log::error("{}", event.error()); }).leak();

    event::GetSongInfo()
        .listen([this](const event::GetSongInfoData& event) {
            const std::optional<Nongs*> nongs = this->getNongs(event.gdId());
            if (!nongs.has_value()) {
                return ListenerResult::Stop;
            }
            SongMetadata* defaultSongMetadata = nongs.value()->defaultSong()->metadata();
            if (event.songName() == defaultSongMetadata->name && event.artistName() == defaultSongMetadata->artist) {
                return ListenerResult::Stop;
            }

            defaultSongMetadata->name = event.songName();
            defaultSongMetadata->artist = event.artistName();

            if (auto saveRes = this->saveNongs(event.gdId()); saveRes.isErr()) {
                log::error("Failed to save song {} metadata update: {}", event.gdId(), saveRes.unwrapErr());
            }

            return ListenerResult::Propagate;
        })
        .leak();

    event::SongDownloadFinished()
        .listen([this](const event::SongDownloadFinishedData& event) {
            const auto nongsOpt = this->getNongs(event.destination()->metadata()->gdID);

            if (!nongsOpt) {
                return ListenerResult::Propagate;
            }

            Nongs* nongs = nongsOpt.value();

            if (GEODE_UNWRAP_IF_ERR(err, nongs->setActive(event.destination()->metadata()->uniqueID))) {
                log::error("Failed to set newly downloaded song {} as active: {}",
                           event.destination()->metadata()->uniqueID, err);
            }

            return ListenerResult::Propagate;
        })
        .leak();

    log::info("Starting NONG read");

    const std::filesystem::path path = this->baseManifestPath();
    if (!jukebox::fs::exists(path)) {
        log::info("No manifest directory found. Creating...");
        auto createDirRes = jukebox::fs::createDirs(path);
        if (createDirRes.isErr()) {
            log::error("Failed to create the manifest directory: {}", createDirRes.unwrapErr());
        }
    }
    if (const std::filesystem::path nongsPath = this->baseNongsPath(); !jukebox::fs::exists(nongsPath)) {
        auto createDirRes = jukebox::fs::createDirs(nongsPath);
        if (createDirRes.isErr()) {
            log::error("Failed to create nongs directory {}: {}", jukebox::fs::toString(nongsPath), createDirRes.unwrapErr());
        }
    }

    auto iterRes = jukebox::fs::listDir(path);
    if (iterRes.isErr()) {
        log::error("Failed to iterate directory {}: {}", jukebox::fs::toString(path), iterRes.unwrapErr());
        return false;
    }

    for (const auto& entryPath : iterRes.unwrap()) {
        if (jukebox::fs::extension(entryPath) != ".json") {
            continue;
        }

        auto res = this->loadNongsFromPath(entryPath);
        if (res.isErr()) {
            const std::string name = jukebox::fs::filename(entryPath);
            log::error("Failed to read file {}: {}", name, res.unwrapErr());
            auto renameRes = jukebox::fs::rename(entryPath, path / jukebox::fs::fromString(fmt::format("{}.bak", name)));
            if (renameRes.isErr()) {
                log::error("Failed to rename file {}: {}", jukebox::fs::toString(entryPath), renameRes.unwrapErr());
            }
            continue;
        }

        std::unique_ptr<Nongs> ptr = std::move(res.unwrap());
        int id = ptr->songID();

        m_manifest.m_nongs.insert({id, std::move(ptr)});
    }

    log::info("Read {} files successfully!", m_manifest.m_nongs.size());

    if (Result<> res = this->migrateV2(); res.isErr()) {
        log::error("{}", res.unwrapErr());
    }

    m_initialized = true;
    return true;
}

Result<> NongManager::migrateV2() {
    if (bool migrate = compat::v2::manifestExists(); !migrate) {
        log::info("Nothing to migrate from V2!");
        return Ok();
    }

    using CompatMap = std::unordered_map<int, compat::CompatManifest>;
    GEODE_UNWRAP_INTO(CompatMap manifest, compat::v2::parseManifest());

    size_t i = 0;

    for (const auto& [id, compatManifest] : manifest) {
        if (!m_manifest.m_nongs.contains(id)) {
            LocalSong defaultSong = compatManifest.defaultSong;
            auto nongs = Nongs(id, std::move(defaultSong));
            m_manifest.m_nongs.insert({id, std::make_unique<Nongs>(std::move(nongs))});

            Nongs* n = m_manifest.m_nongs[id].get();
            IndexManager::get().registerIndexNongs(n);
        }

        Nongs* nongs = m_manifest.m_nongs[id].get();

        for (const LocalSong& song : compatManifest.songs) {
            if (song.path().value() == compatManifest.defaultSong.path().value()) {
                continue;
            }
            bool found = false;
            for (std::unique_ptr<LocalSong>& stored : nongs->locals()) {
                if (stored->metadata()->startOffset == stored->metadata()->startOffset &&
                    stored->metadata()->name == song.metadata()->name &&
                    stored->metadata()->artist == song.metadata()->artist) {
                    found = true;
                }
            }

            if (found) {
                continue;
            }

            LocalSong cp = song;
            auto res = nongs->add(std::move(cp));
            if (res.isErr()) {
                log::error("Failed to add migrated song to manifest: {}", res.unwrapErr());
            }
        }

        if (auto setActiveRes = nongs->setActive(compatManifest.active.metadata()->uniqueID); setActiveRes.isErr()) {
            log::error("Failed to set migrated active song for id {}: {}", id, setActiveRes.unwrapErr());
        }

        if (auto commitRes = nongs->commit(); commitRes.isErr()) {
            log::error("Failed to commit migrated songs for id {}: {}", id, commitRes.unwrapErr());
        }
        i++;
    }

    log::info("Migrated {} ids from v2", i);
    GEODE_UNWRAP(compat::v2::backupManifest(true));

    return Ok();
}

Result<> NongManager::saveNongs(std::optional<int> saveID) {
    const std::filesystem::path path = this->baseManifestPath();

    if (!jukebox::fs::exists(path)) {
        auto createDirRes = jukebox::fs::createDirs(path);
        if (createDirRes.isErr()) {
            return Err("Failed to create the manifest directory: {}", createDirRes.unwrapErr());
        }
    }

    for (const auto& entry : m_manifest.m_nongs) {
        auto id = entry.first;

        if (saveID.has_value() && saveID.value() != id) {
            continue;
        }

        GEODE_UNWRAP(entry.second->commit());
    }

    return Ok();
}

Result<std::unique_ptr<Nongs>> NongManager::loadNongsFromPath(const std::filesystem::path& path) {
    auto stem = jukebox::fs::stem(path);
    GEODE_UNWRAP_INTO(int id, geode::utils::numFromString<int>(stem));

    if (id == 0) {
        return Err("Invalid filename {}", jukebox::fs::filename(path));
    }

    GEODE_UNWRAP_INTO(matjson::Value json, jukebox::fs::readJson(path).mapErr([id](std::string err) {
        return fmt::format("Couldn't parse JSON from file: {}", err);
    }));

    GEODE_UNWRAP_INTO(Nongs nongs, matjson::Serialize<Nongs>::fromJson(json, id).mapErr(
                                       [id](std::string err) { return fmt::format("Failed to parse JSON: {}", err); }));

    return Ok(std::make_unique<Nongs>(std::move(nongs)));
}

bool NongManager::isNongVerifiedForLevelSong(const int levelID, int songID, const std::string_view uniqueID) {
    // List the verified nongs for the given level and song
    std::vector<std::string> verifiedNongs = NongManager::get().getVerifiedNongsForLevel(levelID, {songID});
    return std::ranges::find(verifiedNongs, uniqueID) != verifiedNongs.end();
}

std::string NongManager::getFormattedSize(const std::filesystem::path& path) {
    const auto size = jukebox::fs::fileSize(path);
    if (!size.has_value()) {
        return "N/A";
    }
    double toMegabytes = static_cast<double>(size.value()) / 1024.f / 1024.f;
    return fmt::format("{:.2f}MB", toMegabytes);
}

arc::Future<std::string> NongManager::getMultiAssetSizes(std::string songs, std::string sfx,
                                                         const std::filesystem::path resourcesDir,
                                                         const std::filesystem::path songDir) {
    uintmax_t sum = 0;

    for (auto s : asp::iter::split(std::string_view(songs), ',')) {
        Result<int> idRes = geode::utils::numFromString<int>(s);
        if (idRes.isErr()) {
            continue;
        }
        int id = idRes.unwrap();
        auto result = this->getNongs(id);
        if (!result.has_value()) {
            continue;
        }
        auto nongs = result.value();
        if (nongs->isDefaultActive()) {
            auto songObj = co_await async::waitForMainThread(
                [id]() { return jukebox::gd::getSongInfoObject(id); });
            if (songObj && songObj.value() && songObj.value()->m_fileSize > 0.f) {
                double filesizeMB = songObj.value()->m_fileSize;
                sum += static_cast<uintmax_t>(filesizeMB * 1000000.f);
                continue;
            }
        }

        auto path = nongs->active()->path().value();
        if (jukebox::fs::toString(path).starts_with("songs/")) {
            path = resourcesDir / path;
        }
        if (jukebox::fs::exists(path)) {
            sum += jukebox::fs::fileSize(path).value_or(0);
        }
    }

    for (auto s : asp::iter::split(std::string_view(sfx), ',')) {
        std::string filename = fmt::format("s{}.ogg", s);
        auto localPath = resourcesDir / "sfx" / jukebox::fs::fromString(filename);

        if (jukebox::fs::exists(localPath)) {
            sum += jukebox::fs::fileSize(localPath).value_or(0);
            continue;
        }
        auto path = songDir / jukebox::fs::fromString(filename);
        if (jukebox::fs::exists(path)) {
            sum += jukebox::fs::fileSize(path).value_or(0);
        }
    }

    double toMegabytes = static_cast<double>(sum) / 1000000.f;
    co_return fmt::format("{:.2f}MB", toMegabytes);
}

void NongManager::refetchDefault(const int songID) {
    MusicDownloadManager::sharedState()->clearSong(songID);
    jukebox::gd::getSongInfo(songID, true);
}

Result<> NongManager::addNongs(Nongs&& nongs) {
    if (!m_manifest.m_nongs.contains(nongs.songID())) {
        return Err("Song not initialized in manifest");
    }

    auto& manifestNongs = m_manifest.m_nongs.at(nongs.songID());
    if (auto err = manifestNongs->merge(std::move(nongs)); err.isErr()) {
        return err;
    }
    return saveNongs(manifestNongs->songID());
}

Result<> NongManager::setActiveSong(int gdSongID, std::string uniqueID) {
    auto nongs = getNongs(gdSongID);
    if (!nongs.has_value()) {
        return Err("Song not initialized in manifest");
    }
    if (auto err = nongs.value()->setActive(uniqueID); err.isErr()) {
        return err;
    }
    return saveNongs(gdSongID);
}

Result<> NongManager::deleteSong(int gdSongID, std::string uniqueID) {
    std::optional<Nongs*> nongs = this->getNongs(gdSongID);
    if (!nongs.has_value()) {
        return Err("Song not initialized in manifest");
    }

    GEODE_UNWRAP(nongs.value()->deleteSong(uniqueID).mapErr(
        [](std::string err) { return fmt::format("Couldn't delete Nong: {}", err); }));

    return nongs.value()->commit();
}

Result<> NongManager::deleteSongAudio(int gdSongID, std::string uniqueID) {
    auto nongs = getNongs(gdSongID);
    if (!nongs.has_value()) {
        return Err("Song not initialized in manifest");
    }

    GEODE_UNWRAP(nongs.value()->deleteSongAudio(uniqueID).mapErr(
        [](std::string err) { return fmt::format("Couldn't delete Nong: {}", err); }));

    return saveNongs(gdSongID);
}

Result<> NongManager::deleteAllSongs(int gdSongID) {
    auto nongs = this->getNongs(gdSongID);
    if (!nongs.has_value()) {
        return Err("Song not initialized in manifest");
    }
    if (auto err = m_manifest.m_nongs.at(gdSongID)->deleteAllSongs(); err.isErr()) {
        return err;
    }
    return this->saveNongs(gdSongID);
}

std::filesystem::path NongManager::generateSongFilePath(const std::string& extension,
                                                        std::optional<std::string> filename) {
    auto unique = filename.value_or(jukebox::random_string(16));
    auto destination = jukebox::fs::saveDir() / "nongs";
    if (!jukebox::fs::exists(destination)) {
        auto createDirRes = jukebox::fs::createDirs(destination);
        if (createDirRes.isErr()) {
            log::error("Failed to create nongs save directory {}: {}", jukebox::fs::toString(destination),
                       createDirRes.unwrapErr());
        }
    }
    unique += extension;
    destination = destination / jukebox::fs::fromString(unique);
    return destination;
}

};  // namespace jukebox

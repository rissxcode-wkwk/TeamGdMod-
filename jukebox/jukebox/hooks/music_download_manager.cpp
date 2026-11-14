#include <jukebox/utils/platform.hpp>
#include <vector>
#include <jukebox/hooks/music_download_manager.hpp>

#include <optional>

#include <Geode/binding/MusicDownloadManager.hpp>
#include <Geode/binding/SongInfoObject.hpp>
#include <string>
#include <Geode/loader/Log.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/utils/string.hpp>
#include <jukebox/utils/fs.hpp>

#include <jukebox/events/get_song_info.hpp>
#include <jukebox/managers/nong_manager.hpp>
#include <jukebox/nong/nong.hpp>

using namespace jukebox;

#ifdef JB_IS_MACOS
// Bindings macOS GD 2.2091 tidak mengekspor typeinfo MusicDownloadManager, tetapi
// Modify<> (FieldIntermediate::self) mereferensikannya saat link. Fields tidak
// dipakai (self() tidak pernah dipanggil), jadi cukup sediakan simbol placeholder.
asm(".globl __ZTI20MusicDownloadManager\n"
    ".section __DATA,__const\n"
    ".p2align 3\n"
    "__ZTI20MusicDownloadManager:\n"
    ".quad 0\n"
    ".quad 0\n"
    ".quad 0\n"
    ".text\n");
#endif

// iOS: binding GD 2.2091 belum punya alamat fungsi ini, jadi hook dinonaktifkan di iOS
#ifndef JB_IS_IOS
// Global flag instead of Modify Fields (Fields needs typeinfo for MusicDownloadManager, missing on macOS)
static bool s_overrideSongInfo = false;

std::string JBMusicDownloadManager::pathForSong(int id) {
    NongManager::get().m_currentlyPreparingNong = std::nullopt;
    std::optional<Nongs*> nongs = NongManager::get().getNongs(id);
    if (!nongs.has_value()) {
        return MusicDownloadManager::pathForSong(id);
    }
    Nongs* value = nongs.value();
    Song* active = value->active();
    if (!jukebox::fs::exists(active->path().value())) {
        return MusicDownloadManager::pathForSong(id);
    }
    NongManager::get().m_currentlyPreparingNong = value;

    return jukebox::fs::toString(active->path().value());
}

void JBMusicDownloadManager::onGetSongInfoCompleted(std::string p1, std::string p2) {
    s_overrideSongInfo = true;
    MusicDownloadManager::onGetSongInfoCompleted(p1, p2);
    geode::Result<int> songIDRes = geode::utils::numFromString<int>(p2);
    if (songIDRes.isErr()) {
        return;
    }

    int songID = songIDRes.unwrap();

    SongInfoObject* obj = this->getSongInfoObject(songID);
    s_overrideSongInfo = false;

    if (obj == nullptr) {
        return;
    }

    event::GetSongInfo(songID).send(event::GetSongInfoData{songID, obj->m_songName, obj->m_artistName});
}

SongInfoObject* JBMusicDownloadManager::getSongInfoObject(int id) {
    auto og = MusicDownloadManager::getSongInfoObject(id);
    if (og == nullptr) {
        return og;
    }

    if (s_overrideSongInfo) {
        return og;
    }

    std::optional<Nongs*> opt = NongManager::get().getNongs(id);

    if (!opt) {
        return og;
    }

    Nongs* res = opt.value();
    Song* active = res->active();

    og->m_songName = active->metadata()->name;
    og->m_artistName = active->metadata()->artist;
    return og;
}
#endif  // JB_IS_IOS

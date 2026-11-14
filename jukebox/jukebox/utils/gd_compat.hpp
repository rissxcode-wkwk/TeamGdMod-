#pragma once

#include <filesystem>
#include <string>

#include <Geode/Geode.hpp>
#include <jukebox/utils/platform.hpp>
#include <Geode/binding/LevelTools.hpp>
#include <Geode/binding/MusicDownloadManager.hpp>
#include <Geode/binding/SongInfoObject.hpp>

// Wrapper agar iOS (bindings GD 2.2091 belum punya alamat beberapa fungsi)
// tidak menghasilkan "undefined symbol" saat linking.
#ifdef JB_IS_IOS
#pragma message("jukebox gd_compat: iOS shim ACTIVE")
#else
#pragma message("jukebox gd_compat: native GD calls")
#endif

namespace jukebox::gd {

inline std::string pathForSong(int id) {
#ifdef JB_IS_IOS
    std::string base = std::string(cocos2d::CCFileUtils::sharedFileUtils()->getWritablePath2());
    return (std::filesystem::path(base) / (std::to_string(id) + ".mp3")).string();
#else
    return MusicDownloadManager::sharedState()->pathForSong(id);
#endif
}

inline SongInfoObject* getSongInfoObject(int id) {
#ifdef JB_IS_IOS
    (void)id;
    return nullptr;
#else
    return MusicDownloadManager::sharedState()->getSongInfoObject(id);
#endif
}

inline void getSongInfo(int id, bool refresh) {
#ifdef JB_IS_IOS
    (void)id;
    (void)refresh;
#else
    MusicDownloadManager::sharedState()->getSongInfo(id, refresh);
#endif
}

inline std::string getAudioFileName(int id) {
#ifdef JB_IS_IOS
    (void)id;
    return "";
#else
    return LevelTools::getAudioFileName(id);
#endif
}

}  // namespace jukebox::gd

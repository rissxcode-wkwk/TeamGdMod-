#pragma once

#include <jukebox/utils/platform.hpp>

#include <string>
#include <vector>

#include <Geode/binding/MusicDownloadManager.hpp>
#include <Geode/binding/SongInfoObject.hpp>
#include <string>
#include <Geode/modify/MusicDownloadManager.hpp>  // IWYU pragma: keep

// iOS: binding GD 2.2091 belum punya alamat fungsi ini, jadi hook dinonaktifkan di iOS
#ifndef JB_IS_IOS
struct JBMusicDownloadManager
    : geode::Modify<JBMusicDownloadManager, MusicDownloadManager> {
    std::string pathForSong(int id);
    void onGetSongInfoCompleted(std::string p1, std::string p2);
    SongInfoObject* getSongInfoObject(int id);
};
#endif  // JB_IS_IOS

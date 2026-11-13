#include <optional>

#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/modify/GJGameLevel.hpp>  // IWYU pragma: keep
#include <Geode/modify/Modify.hpp>
#include <Geode/utils/string.hpp>
#include <jukebox/utils/fs.hpp>

#include <jukebox/managers/nong_manager.hpp>

using namespace geode::prelude;
using namespace jukebox;

class $modify(GJGameLevel) {
    std::string getAudioFileName() {
        NongManager::get().m_currentlyPreparingNong = std::nullopt;
        // If we have a custom song, return
        if (m_songID != 0) {
            return GJGameLevel::getAudioFileName();
        }
        int id = (-m_audioTrack) - 1;
        std::optional<Nongs*> res = NongManager::get().getNongs(id);
        if (!res.has_value()) {
            return GJGameLevel::getAudioFileName();
        }
        Song* active = res.value()->active();
        if (!jukebox::fs::exists(active->path().value())) {
            return GJGameLevel::getAudioFileName();
        }
        jukebox::NongManager::get().m_currentlyPreparingNong = res.value();

        return jukebox::fs::toString(active->path().value());
    }
};

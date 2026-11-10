#include <modules/gui/gui.hpp>
#include <modules/hack/hack.hpp>
#include <modules/config/config.hpp>

#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/GameManager.hpp>

namespace eclipse::hacks::Level {

    namespace {
        // Menyimpan objek asli untuk key -142 / -1329 pada satu dictionary animasi
        struct CoinAnimBackup {
            cocos2d::CCObject* secret = nullptr;
            cocos2d::CCObject* user = nullptr;
            bool saved = false;
        };

        void applyShowCoins(cocos2d::CCDictionary* dict, CoinAnimBackup& backup, bool state) {
            if (!dict) return;

            constexpr intptr_t kSecretCoin = 142;
            constexpr intptr_t kUserCoin = 1329;

            if (!backup.saved) {
                backup.secret = dict->objectForKey(-kSecretCoin);
                backup.user = dict->objectForKey(-kUserCoin);
                if (backup.secret) backup.secret->retain();
                if (backup.user) backup.user->retain();
                backup.saved = true;
            }

            if (state) {
                // pakai varian "sudah dikoleksi" yang sama dengan varian normal
                if (auto* secret = dict->objectForKey(kSecretCoin)) dict->setObject(secret, -kSecretCoin);
                if (auto* user = dict->objectForKey(kUserCoin)) dict->setObject(user, -kUserCoin);
            } else {
                if (backup.secret) dict->setObject(backup.secret, -kSecretCoin);
                if (backup.user) dict->setObject(backup.user, -kUserCoin);
            }
        }
    }

    void onChangeShowCoins(bool state) {
        // Geode/GD 2.2091: m_unkAnimationDict sudah tidak ada, data animasi dipecah ke beberapa dictionary
        static CoinAnimBackup mainFrames, detailFrames, frameTime, frames, defaults;

        auto* GM = GameManager::sharedState();
        if (!GM) return;

        applyShowCoins(GM->m_mainFramesForAnimation, mainFrames, state);
        applyShowCoins(GM->m_detailFramesForAnimation, detailFrames, state);
        applyShowCoins(GM->m_frameTimeForAnimation, frameTime, state);
        applyShowCoins(GM->m_framesForAnimation, frames, state);
        applyShowCoins(GM->m_defaultFrames, defaults, state);
    }

    class AlwaysShowCoins : public hack::Hack {
        void init() override {
            auto tab = gui::MenuTab::find("Level");

            tab->addToggle("Always Show Coins", "level.alwaysshowcoins")
                ->handleKeybinds()
                ->setDescription("Always shows the uncollected variant of the coins.")
                ->callback(onChangeShowCoins);
        }

        void lateInit() override {
            onChangeShowCoins(config::get<bool>("level.alwaysshowcoins", false));
        }

        [[nodiscard]] const char* getId() const override { return "Always Show Coins"; }
    };

    REGISTER_HACK(AlwaysShowCoins)

    // if qolmod makes 3 billion playlayers this will not work correctly
    static bool skipUniqueCoin = false;

    class $modify(AlwaysShowCoinsGOHook, GameObject) {
        ADD_HOOKS_DELEGATE("level.alwaysshowcoins")

        void playDestroyObjectAnim(GJBaseGameLayer* p0) {
            skipUniqueCoin = true;
            GameObject::playDestroyObjectAnim(p0);
            skipUniqueCoin = false;
        }
    };

    class $modify(AlwaysShowCoinsBGLHook, GJBaseGameLayer) {
        bool hasUniqueCoin(EffectGameObject* p0) {
            if (!GJBaseGameLayer::hasUniqueCoin(p0))
                return false;

            return !skipUniqueCoin;
        }
    };
}

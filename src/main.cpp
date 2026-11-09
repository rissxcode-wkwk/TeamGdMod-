#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include "FloatingButton.hpp"

using namespace geode::prelude;

// ------------------------------------------------------------ aksi yang dipakai menu & shortcut
namespace tg {
void actRestart() { if (auto p = PlayLayer::get()) p->resetLevel(); }
void actPractice() { if (auto p = PlayLayer::get()) p->togglePracticeMode(!p->m_isPracticeMode); }
// API checkpoint & options berbeda antar versi GD -> dipilih saat compile
template <class P> void placeCP(P* p) {
    if constexpr (requires { p->createCheckpoint(); }) p->createCheckpoint();
    else if constexpr (requires { p->markCheckpoint(); }) p->markCheckpoint();
}
template <class P> void removeCP(P* p) {
    if constexpr (requires { p->removeLastCheckpoint(); }) p->removeLastCheckpoint();
    else if constexpr (requires { p->removeCheckpoint(true); }) p->removeCheckpoint(true);
}
template <class O> void openOptions() {
    if constexpr (requires { O::addToCurrentScene(false); }) O::addToCurrentScene(false);
    else if constexpr (requires { O::create()->show(); }) O::create()->show();
}
void actPlaceCheckpoint() { if (auto p = PlayLayer::get()) if (p->m_isPracticeMode) placeCP(p); }
void actRemoveCheckpoint() { if (auto p = PlayLayer::get()) if (p->m_isPracticeMode) removeCP(p); }
void actShowOptions() { openOptions<OptionsLayer>(); }
void actJump(bool p1) {
    auto p = PlayLayer::get();
    if (!p) return;
    auto pl = p1 ? p->m_player1 : p->m_player2;
    if (!pl) return;
    pl->pushButton(PlayerButton::Jump);
    p->runAction(CCSequence::create(CCDelayTime::create(0.05f),
        CallFuncExt::create([p1] {
            if (auto q = PlayLayer::get()) {
                auto pl2 = p1 ? q->m_player1 : q->m_player2;
                if (pl2) pl2->releaseButton(PlayerButton::Jump);
            }
        }), nullptr));
}
}

// ------------------------------------------------------------ startup
$on_mod(Loaded) {
    tg::loadAll();
    tg::refresh();
    // tandai fitur yang field/API-nya tidak ada di versi GD ini (tampil abu-abu, bukan toggle palsu)
    if (!tg::lab::hasHitboxes<PlayLayer>) tg::g_unsupported.insert("level-show-hitboxes");
    if (!tg::lab::hasAttempts<PlayLayer>) tg::g_unsupported.insert("labels-attempt");
    if (!tg::lab::hasLevelTime<PlayLayer>) tg::g_unsupported.insert("labels-level-time");
    if (!tg::lab::hasPitch<FMODAudioEngine>) {
        tg::g_unsupported.insert("global-pitch-shift");
        tg::g_unsupported.insert("global-audio-speed");
        tg::g_unsupported.insert("global-sync-with-speedhack");
    }
    if (!tg::lab::hasTrail<PlayerObject>) tg::g_unsupported.insert("player-no-trail");
    if (!tg::lab::hasRobotFire<PlayerObject>) tg::g_unsupported.insert("player-no-robot-fire");
    if (!tg::lab::hasGhost<PlayerObject>) tg::g_unsupported.insert("player-no-ghost-trail");
    if (!tg::lab::hasSolid<PlayerObject>) tg::g_unsupported.insert("player-solid-wave-trail");
    if (!tg::lab::hasGround<PlayerObject>) tg::g_unsupported.insert("player-jump-hack");
    if (!tg::lab::hasVehParts<PlayerObject>) tg::g_unsupported.insert("player-no-vehicle-particles");
    if (!tg::lab::hasPctLabel<PlayLayer>) tg::g_unsupported.insert("level-accurate-percentage");
    listenForSettingChanges<double>("button-size", [](double) { tg::FloatingButton::refresh(); });
    listenForSettingChanges<double>("button-opacity", [](double) { tg::FloatingButton::refresh(); });
}

$execute {
    using namespace tg;
    listenForKeybindSettingPresses("open-menu", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        if (on("keybinds-in-game-only") && !PlayLayer::get()) return;
        toggleMenu();
    });
    listenForKeybindSettingPresses("open-palette", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) openPalette();
    });
    listenForKeybindSettingPresses("p1-jump", [](Keybind const&, bool down, bool repeat, double) {
        if (repeat) return;
        auto p = PlayLayer::get();
        if (!p || !p->m_player1) return;
        if (down) p->m_player1->pushButton(PlayerButton::Jump); else p->m_player1->releaseButton(PlayerButton::Jump);
    });
    listenForKeybindSettingPresses("p2-jump", [](Keybind const&, bool down, bool repeat, double) {
        if (repeat) return;
        auto p = PlayLayer::get();
        if (!p || !p->m_player2) return;
        if (down) p->m_player2->pushButton(PlayerButton::Jump); else p->m_player2->releaseButton(PlayerButton::Jump);
    });
    listenForKeybindSettingPresses("show-options", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat && PlayLayer::get()) actShowOptions();
    });
    listenForKeybindSettingPresses("restart-level", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) actRestart();
    });
    listenForKeybindSettingPresses("toggle-practice", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) actPractice();
    });
    listenForKeybindSettingPresses("place-checkpoint", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) actPlaceCheckpoint();
    });
    listenForKeybindSettingPresses("remove-checkpoint", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) actRemoveCheckpoint();
    });
    listenForKeybindSettingPresses("step-frame", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) g_stepPending = true;
    });
}

// ------------------------------------------------------------ hooks
// Floating button dibuat sekali, hidup lintas scene.
class $modify(TGMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        tg::FloatingButton::ensure();
#if !TG_HAS_OVERLAY
        tg::FloatingButton::attach();
#endif
        return true;
    }
};

// Speedhack, Frame Stepper, pitch audio
class $modify(TGScheduler, CCScheduler) {
    void update(float dt) {
        if (dt > 0.f) tg::lab::fps = tg::lab::fps * 0.9f + (1.f / dt) * 0.1f;
        auto& c = tg::cache();
        float d = dt;
        if (!tg::g_menuOpen) {
            d = dt * c.speed;
            if (c.stepper && PlayLayer::get()) {
                if (tg::g_stepPending) { d = 1.f / 60.f; tg::g_stepPending = false; }
                else d = 0.f;
            }
        }
        CCScheduler::update(d);

        static float lastPitch = 1.f;
        static int tick = 0;
        if (c.pitch != lastPitch || (++tick % 90 == 0 && c.pitch != 1.f)) {
            if (auto e = FMODAudioEngine::get()) { tg::lab::setPitch(e, c.pitch); lastPitch = c.pitch; }
        }
    }
};

class $modify(TGPlayLayer, PlayLayer) {
    struct Fields { CCLabelBMFont* labels = nullptr; int clickPhase = 0; bool killed = false; };

    void createLabels() {
        if (m_fields->labels) return;
        auto win = CCDirector::get()->getWinSize();
        auto l = CCLabelBMFont::create("", "chatFont.fnt");
        l->setAnchorPoint({0.f, 1.f});
        l->setAlignment(kCCTextAlignmentLeft);
        l->setPosition({6.f, win.height - 6.f});
        l->setID("tg-labels"_spr);
        if (m_uiLayer) m_uiLayer->addChild(l, 50);
        m_fields->labels = l;
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        tg::lab::resetRun();
        tg::bot::frame = 0;
        tg::bot::idx = 0;
        if (tg::cache().botRec) tg::bot::rec.clear();
        createLabels();
        if (tg::on("level-auto-ldm")) GameManager::get()->setGameVariable("0042", true); // Low Detail Mode, berlaku di level berikutnya
        if (tg::cache().autoPractice && !m_isPracticeMode) this->togglePracticeMode(true);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->killed = false;
        tg::bot::frame = 0;
        tg::bot::idx = 0;
        tg::lab::resetRun();
        if (tg::cache().botRec) tg::bot::rec.clear();
    }

    // Noclip
    void destroyPlayer(PlayerObject* p, GameObject* obj) {
        if (tg::cache().noclip && obj != m_anticheatSpike) {
            tg::lab::hitThisFrame = true;
            return;
        }
        PlayLayer::destroyPlayer(p, obj);
    }

    // Freeze Attempts
    void updateAttempts() {
        if (tg::cache().freeze) return;
        PlayLayer::updateAttempts();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& c = tg::cache();
        ++tg::bot::frame;
        ++tg::lab::frames;
        tg::lab::tickHit();

        // Safe Mode / Auto Safe Mode: level berjalan sebagai test mode (progres tidak tersimpan)
        if (c.safe()) m_isTestMode = true;
        if (c.hitboxes) tg::lab::forceHitboxes(this);
        if (c.noTrail || c.noRobot || c.noGhost || c.solidWave || c.jumpHack || c.noVehPart) tg::lab::visualTweaks(this);
        if (c.accPct) tg::lab::accuratePercent(this);

        // Auto Kill: mati sekali per percobaan saat mencapai persen target
        if (c.autoKill && !m_fields->killed && m_player1 && this->getCurrentPercent() >= tg::get("player-auto-kill-percent")) {
            m_fields->killed = true;
            this->destroyPlayer(m_player1, m_anticheatSpike);
        }

        // AutoClicker (P1)
        if (c.autoclick && m_player1) {
            int ph = ++m_fields->clickPhase;
            if (ph % c.clickInterval == 0) m_player1->pushButton(PlayerButton::Jump);
            else if (ph % c.clickInterval == std::max(1, c.clickInterval / 2)) m_player1->releaseButton(PlayerButton::Jump);
        }

        // Bot playback
        if (c.botPlay) {
            while (tg::bot::idx < tg::bot::rec.size() && tg::bot::rec[tg::bot::idx].frame <= tg::bot::frame) {
                auto& e = tg::bot::rec[tg::bot::idx++];
                auto pl = e.p1 ? m_player1 : m_player2;
                if (!pl) continue;
                if (e.down) pl->pushButton(PlayerButton::Jump); else pl->releaseButton(PlayerButton::Jump);
            }
        }

        // Labels
        if (m_fields->labels) {
            bool show = tg::on("labels-show-labels");
            m_fields->labels->setVisible(show);
            if (show) {
                m_fields->labels->setString(tg::lab::text(this).c_str());
                m_fields->labels->setScale(tg::get("labels-labels-scale"));
                m_fields->labels->setOpacity((GLubyte)(tg::get("labels-labels-opacity") * 255));
            }
        }
    }
};

// Rekam input untuk Bot + hitung CPS
class $modify(TGPlayer, PlayerObject) {
    bool pushButton(PlayerButton b) {
        if (b == PlayerButton::Jump) {
            if (auto p = PlayLayer::get()) {
                if (this == p->m_player1 || this == p->m_player2) {
                    tg::lab::click();
                    if (tg::cache().botRec) tg::bot::rec.push_back({tg::bot::frame, true, this == p->m_player1});
                }
            }
        }
        return PlayerObject::pushButton(b);
    }
    bool releaseButton(PlayerButton b) {
        if (b == PlayerButton::Jump && tg::cache().botRec) {
            if (auto p = PlayLayer::get())
                if (this == p->m_player1 || this == p->m_player2)
                    tg::bot::rec.push_back({tg::bot::frame, false, this == p->m_player1});
        }
        return PlayerObject::releaseButton(b);
    }
    void playDeathEffect() {
        if (tg::cache().noDeathFx) return;
        PlayerObject::playDeathEffect();
    }
};

// Unlock Icons
class $modify(TGGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (tg::cache().unlock) return true;
        return GameManager::isIconUnlocked(id, type);
    }
};

// No Mirror: portal cermin diabaikan
class $modify(TGBaseLayer, GJBaseGameLayer) {
    void toggleFlipped(bool flipX, bool flipY) {
        if (tg::cache().noMirror) return;
        GJBaseGameLayer::toggleFlipped(flipX, flipY);
    }
};

// Hide Pause Menu: sembunyikan menu pause (tombol Back/Esc tetap melanjutkan game)
class $modify(TGPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (tg::cache().hidePause) this->setVisible(false);
    }
};

// Unlock Shops / Unlock Vaults: semua item dianggap terbuka
class $modify(TGStats, GameStatsManager) {
    bool isItemUnlocked(UnlockType type, int id) {
        if (tg::cache().unlockShops) return true;
        return GameStatsManager::isItemUnlocked(type, id);
    }
};

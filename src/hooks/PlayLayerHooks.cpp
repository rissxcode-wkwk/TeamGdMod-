// Hook PlayLayer: Noclip, Mode Aman, HUD, checkpoint otomatis, respawn cepat, statistik permanen,
// dan siklus hidup Replay Bot.

#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <string>

#include "../bot/Bot.hpp"
#include "../core/Cheats.hpp"
#include "../core/Config.hpp"
#include "../core/Db.hpp"
#include "../core/Riwayat.hpp"
#include "../core/Stats.hpp"
#include "../hacks/Cbf.hpp"
#include "../hacks/Extra.hpp"
#include "../hacks/Stepper.hpp"
#include <vector>

using namespace geode::prelude;

namespace {
    // Jam saat ini dalam format HH:MM
    std::string jamSekarang() {
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm lokal{};
#if defined(GEODE_IS_WINDOWS)
        localtime_s(&lokal, &t);
#else
        localtime_r(&t, &lokal);
#endif
        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M", &lokal);
        return buf;
    }

    int idLevel(PlayLayer* pl) {
        return pl->m_level ? pl->m_level->m_levelID.value() : 0;
    }

    int persenTerbaik(PlayLayer* pl) {
        if (!pl->m_level) return 0;
        return pl->m_isPracticeMode ? pl->m_level->m_practicePercent : pl->m_level->m_normalPercent.value();
    }

    // Menyusun teks HUD dari status saat ini. Setiap baris hanya muncul bila diaktifkan / relevan.
    std::string susunTeksHud(PlayLayer* pl, bool modeAman) {
        auto const& c = tgm::cfg();
        auto const& s = tgm::stats();
        auto frame = pl->m_gameState.m_currentProgress;
        std::string t;

        if (c.showLevelName && pl->m_level) t += fmt::format("{}\n", std::string(pl->m_level->m_levelName));
        if (c.showFps)     t += fmt::format("FPS: {:.0f}\n", s.fps);
        if (c.showCps)     t += fmt::format("CPS: {}\n", s.cps());
        if (c.showClock)   t += fmt::format("Jam: {}\n", jamSekarang());
        if (c.showPercent) t += c.persenAkurat ? fmt::format("Persen: {:.2f}%\n", pl->getCurrentPercent()) : fmt::format("Persen: {:.1f}%\n", pl->getCurrentPercent());
        if (c.showBest)    t += fmt::format("Terbaik: {}%\n", persenTerbaik(pl));
        if (c.showMatiTerakhir && s.persenMatiTerakhir >= 0.f) t += fmt::format("Mati di: {:.0f}%\n", s.persenMatiTerakhir);
        if (c.showAttempt) t += fmt::format("Percobaan: {}\n", pl->m_attempts);
        if (c.showTime)    t += fmt::format("Waktu: {:.1f} dtk\n", pl->m_gameState.m_levelTime);
        if (c.showFrame)   t += fmt::format("Frame: {}\n", frame);
        if (c.showClicks)  t += fmt::format("Klik: {}\n", s.klik);
        if (c.showDeaths)  t += fmt::format("Mati (sesi): {}\n", tgm::sesi().mati);
        if (c.noclip) {
            if (c.batasTabrakan > 0) {
                t += fmt::format("Noclip: {}/{} tabrakan\n", s.tabrakan, c.batasTabrakan);
            } else {
                t += fmt::format("Noclip: {} tabrakan\n", s.tabrakan);
            }
            if (c.showAkurasi) t += fmt::format("Akurasi: {:.1f}%\n", s.akurasi(frame));
        }
        if (std::abs(c.speed - 1.f) > 0.001f) {
            t += fmt::format("Kecepatan: {:.1f}x\n", c.speed);
        }

        switch (tgm::bot::mode()) {
            case tgm::bot::Mode::Rekam:
                t += fmt::format("Bot: Merekam ({} aksi)\n", tgm::bot::jumlahAksi());
                break;
            case tgm::bot::Mode::Putar:
                t += fmt::format("Bot: Memutar ({}/{})\n", tgm::bot::posisi(), tgm::bot::jumlahAksi());
                break;
            default:
                break;
        }

        if (c.showTestmode && pl->m_isTestMode) t += "Testmode\n";
        if (c.showBestRun && s.bestRunB > s.bestRunA) t += fmt::format("Best Run: {:.0f}-{:.0f}%\n", s.bestRunA, s.bestRunB);
        if (c.noclip && c.showNoclipMati) t += fmt::format("Noclip Mati: {}\n", s.matiNoclip);

        if (c.showCheat && tgm::cheatsActive()) t += "CHEAT AKTIF\n";
        if (modeAman) t += "Mode Aman aktif\n";
        if (!t.empty()) t.pop_back(); // buang newline terakhir
        return t;
    }

    // Warna HUD yang bisa dipilih (indeks = Config::hudColor)
    ccColor3B warnaHud(int indeks) {
        static ccColor3B const daftar[] = {
            {255, 255, 255}, // putih
            {56, 208, 255},  // cyan
            {61, 220, 151},  // hijau
            {255, 209, 102}, // kuning
            {255, 99, 99},   // merah
            {255, 150, 60},  // oranye
            {176, 132, 255}, // ungu
            {255, 140, 200}, // pink
        };
        return daftar[std::clamp(indeks, 0, 7)];
    }

    char const* fileFont(int indeks) {
        static char const* const daftar[] = {"chatFont.fnt", "bigFont.fnt", "goldFont.fnt"};
        return daftar[std::clamp(indeks, 0, 2)];
    }

    // Menempatkan label HUD sesuai sudut, ukuran, warna, font, dan kepekatan pilihan, plus latar gelapnya
    void terapkanGayaHud(CCLabelBMFont* label, CCLayerColor* latar, int& fontSaatIni) {
        auto const& c = tgm::cfg();
        auto win = CCDirector::get()->getWinSize();

        if (fontSaatIni != c.hudFont) {
            label->setFntFile(fileFont(c.hudFont));
            fontSaatIni = c.hudFont;
        }

        bool kanan = (c.hudCorner == 1 || c.hudCorner == 3);
        bool tengah = (c.hudCorner == 4 || c.hudCorner == 5);
        bool bawah = (c.hudCorner == 2 || c.hudCorner == 3 || c.hudCorner == 5);

        float ax = tengah ? 0.5f : (kanan ? 1.f : 0.f);
        label->setAnchorPoint({ax, bawah ? 0.f : 1.f});
        label->setAlignment(tengah ? kCCTextAlignmentCenter : (kanan ? kCCTextAlignmentRight : kCCTextAlignmentLeft));

        float x = tengah ? win.width / 2.f : (kanan ? win.width - 10.f : 10.f);
        float y = bawah ? 8.f : win.height - 8.f;
        label->setPosition({x + static_cast<float>(c.hudOffX), y + static_cast<float>(c.hudOffY)});

        // Font besar/emas lebih lebar, jadi diperkecil agar ukuran terlihat sebanding
        float dasar = c.hudFont == 0 ? 0.55f : 0.32f;
        label->setScale(dasar * static_cast<float>(c.hudScale) / 100.f);
        label->setColor(warnaHud(c.hudColor));
        label->setOpacity(static_cast<GLubyte>(std::clamp(c.hudOpacity, 10, 100) * 255 / 100));

        if (latar) {
            auto kotak = label->boundingBox();
            bool ada = c.hudPanel && label->isVisible() && kotak.size.width > 1.f;
            latar->setVisible(ada);
            if (ada) {
                constexpr float PAD = 4.f;
                latar->setContentSize({kotak.size.width + PAD * 2.f, kotak.size.height + PAD * 2.f});
                latar->setPosition({kotak.origin.x - PAD, kotak.origin.y - PAD});
                latar->setOpacity(static_cast<GLubyte>(std::clamp(c.hudOpacity, 10, 100) * 110 / 100));
            }
        }
    }

    // ---------- Input Viewer ----------
    constexpr float UKURAN_KOTAK = 26.f;
    constexpr float JARAK_KOTAK = 4.f;
    constexpr int   TAG_KOTAK = 100;   // tag kotak = TAG_KOTAK + indeks tombol (0..5): P1 [kiri, lompat, kanan], P2 [kiri, lompat, kanan]

    // Keadaan terakhir yang sudah diterapkan ke node. Bila tidak berubah, frame ini tidak menyentuh node sama sekali.
    struct IvKeadaan {
        bool tampil = false;
        bool dua = false;
        int pos = -1, ukuran = -1, opas = -1, warna = -1, offX = 0, offY = 0;
        unsigned mask = 0xFFFFFFFFu;
        float lebar = 0.f, tinggi = 0.f;
        bool operator==(IvKeadaan const&) const = default;
    };

    // Hapus semua anak dengan ID tertentu (mencegah node ganda bila setup berjalan lebih dari sekali)
    void bersihkan(CCNode* induk, std::string const& id) {
        while (auto* n = induk->getChildByID(id)) n->removeFromParent();
    }

    // Status tombol AKTUAL dari objek pemain GD (bukan dari parameter event), jadi P1 dan P2 tidak bisa tertukar
    // dan tidak ada status "tersangkut": pemain mati atau level dijeda selalu dianggap dilepas.
    bool tombolDitekan(PlayLayer* pl, PlayerObject* p, int tombol) {
        if (!p || p->m_isDead || pl->m_isPaused) return false;
        auto it = p->m_holdingButtons.find(tombol);
        if (it != p->m_holdingButtons.end() && it->second) return true;
        if (tombol == 2) return p->m_holdingLeft;
        if (tombol == 3) return p->m_holdingRight;
        return false;
    }

    CCNode* bangunInputViewer(PlayLayer* pl) {
        auto* akar = CCNode::create();
        akar->setID("tgm-iv"_spr);
        akar->ignoreAnchorPointForPosition(false);

        bool platformer = pl->m_isPlatformer;
        int kolom = platformer ? 3 : 1;
        char const* teksPlatformer[3] = {"<", "^", ">"};

        for (int p = 0; p < 2; ++p) {
            for (int k = 0; k < kolom; ++k) {
                float x = static_cast<float>(k) * (UKURAN_KOTAK + JARAK_KOTAK);
                float y = static_cast<float>(1 - p) * (UKURAN_KOTAK + JARAK_KOTAK);
                auto* kotak = CCLayerColor::create(ccc4(255, 255, 255, 255), UKURAN_KOTAK, UKURAN_KOTAK);
                kotak->setPosition({x, y});
                kotak->setTag(TAG_KOTAK + p * 3 + (platformer ? k : 1));
                akar->addChild(kotak);

                auto* label = CCLabelBMFont::create(platformer ? teksPlatformer[k] : (p == 0 ? "P1" : "P2"), "bigFont.fnt");
                label->setScale(0.42f);
                label->setPosition({UKURAN_KOTAK / 2.f, UKURAN_KOTAK / 2.f});
                kotak->addChild(label);
            }
        }
        akar->setContentSize({static_cast<float>(kolom) * UKURAN_KOTAK + static_cast<float>(kolom - 1) * JARAK_KOTAK,
                              UKURAN_KOTAK * 2.f + JARAK_KOTAK});
        return akar;
    }

    void perbaruiInputViewer(CCNode* akar, PlayLayer* pl, IvKeadaan& cache) {
        auto const& c = tgm::cfg();
        auto win = CCDirector::get()->getWinSize();
        bool dua = pl->m_gameState.m_isDualMode || (pl->m_levelSettings && pl->m_levelSettings->m_twoPlayerMode);

        unsigned mask = 0;
        if (c.ivTampil) {
            static int const URUTAN[3] = {2, 1, 3};   // tombol GD untuk [kiri, lompat, kanan]
            for (int i = 0; i < 3; ++i) {
                if (tombolDitekan(pl, pl->m_player1, URUTAN[i])) mask |= 1u << i;
                if (dua && tombolDitekan(pl, pl->m_player2, URUTAN[i])) mask |= 1u << (3 + i);
            }
        }

        IvKeadaan baru{c.ivTampil, dua, c.ivPosisi, c.ivUkuran, c.ivOpasitas, c.ivWarna, c.ivOffX, c.ivOffY, mask, win.width, win.height};
        if (baru == cache) return;
        cache = baru;

        akar->setVisible(c.ivTampil);
        if (!c.ivTampil) return;

        bool kanan = (c.ivPosisi == 1 || c.ivPosisi == 3);
        bool tengah = (c.ivPosisi == 4 || c.ivPosisi == 5);
        bool bawah = (c.ivPosisi == 2 || c.ivPosisi == 3 || c.ivPosisi == 5);
        akar->setAnchorPoint({tengah ? 0.5f : (kanan ? 1.f : 0.f), bawah ? 0.f : 1.f});
        akar->setScale(static_cast<float>(c.ivUkuran) / 100.f);
        akar->setPosition({(tengah ? win.width / 2.f : (kanan ? win.width - 12.f : 12.f)) + static_cast<float>(c.ivOffX),
                           (bawah ? 12.f : win.height - 12.f) + static_cast<float>(c.ivOffY)});

        float opasitas = static_cast<float>(c.ivOpasitas) / 100.f;
        auto warna = warnaHud(c.ivWarna);
        for (int i = 0; i < 6; ++i) {
            auto* kotak = typeinfo_cast<CCLayerColor*>(akar->getChildByTag(TAG_KOTAK + i));
            if (!kotak) continue;
            kotak->setVisible(i < 3 || dua);
            bool ditekan = (mask >> i) & 1u;
            kotak->setColor(ditekan ? warna : ccColor3B{40, 40, 40});
            kotak->setOpacity(static_cast<GLubyte>((ditekan ? 1.f : 0.55f) * opasitas * 255.f));
        }
    }

    void kabari(std::string const& pesan, NotificationIcon ikon) {
        Notification::create(pesan, ikon, 1.8f)->show();
    }

    // ---------- Jejak lintasan ----------
    // Menggambar ulang seluruh jejak (dipanggil hanya saat ada titik baru)
    void gambarJejak(CCDrawNode* node, std::vector<CCPoint> const& a, std::vector<CCPoint> const& b) {
        if (!node) return;
        node->clear();
        auto w = warnaHud(tgm::cfg().jejakWarna);
        ccColor4F warna{static_cast<float>(w.r) / 255.f, static_cast<float>(w.g) / 255.f, static_cast<float>(w.b) / 255.f, 0.9f};
        for (auto const* daftar : {&a, &b}) {
            for (std::size_t i = 1; i < daftar->size(); ++i) {
                auto const& p = (*daftar)[i - 1];
                auto const& q = (*daftar)[i];
                // Lompatan jauh (portal teleport, mulai dari checkpoint) tidak digambar sebagai garis
                if (std::abs(p.x - q.x) > 150.f || std::abs(p.y - q.y) > 150.f) continue;
                node->drawSegment(p, q, 0.9f, warna);
            }
        }
    }

    void potongJejak(std::vector<CCPoint>& v, std::size_t maks) {
        if (v.size() > maks) v.erase(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(v.size() - maks));
    }

    // LDM otomatis: nilai asli pengaturan GD disimpan lalu dikembalikan saat keluar level
    bool g_ldmDiubah = false;
    bool g_ldmAsli = false;
}

class $modify(TgmPlayLayer, PlayLayer) {
    struct Fields {
        std::string teksTerakhir;      // cache agar setString tidak dipanggil tiap frame
        float       akumulasiHud = 0.f;
        float       sejakFlush = 0.f;  // waktu nyata sejak database terakhir di-flush
        int         fontSaatIni = -1;
        bool        menungguReset = false;   // ada kematian yang belum diikuti reset level
        CCNode*     inputViewer = nullptr;   // dimiliki m_uiLayer
        IvKeadaan   ivCache;                 // keadaan Input Viewer yang terakhir diterapkan
        bool        hitboxTerapkan = false;  // nilai Config::hitbox yang terakhir diterapkan ke GD
        CCDrawNode* jejakNode = nullptr;     // dimiliki m_objectLayer
        std::vector<CCPoint> jejak1;         // titik jejak pemain 1
        std::vector<CCPoint> jejak2;         // titik jejak pemain 2
        int         jejakAkum = 0;           // penghitung frame untuk jarak antar titik
        bool        autoKillSudah = false;   // Auto Kill sudah dipicu pada percobaan ini
        bool        musikDijeda = false;     // musik dijeda oleh Mute Audio Saat Mati
    };

    // LDM otomatis (atau Mode Ringan) harus menyala sebelum objek level dibuat
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        auto const& c = tgm::cfg();
        tgm::terapkanFps();
        if ((c.autoLdm || c.modeRingan) && !g_ldmDiubah) {
            auto* gm = GameManager::get();
            g_ldmAsli = gm->getGameVariable("0042");
            gm->setGameVariable("0042", true);
            g_ldmDiubah = true;
        }
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        auto& st = tgm::stats();
        st = tgm::Stats{};

        int id = idLevel(this);
        if (m_level) tgm::riwayat::catatNama(id, std::string(m_level->m_levelName));

        // Ikat bot ke level ini (macro level lain dibuang), lalu muat slot terpilih bila diminta
        tgm::bot::ikatLevel(id);
        if (tgm::cfg().autoMuat) {
            std::string pesan;
            tgm::bot::muat(id, tgm::cfg().slot, pesan);
        }

        // Mode Aman: cheat aktif (atau Safe Mode manual) -> progres tidak disimpan
        if (tgm::perluModeAman()) {
            m_isTestMode = true;
        }

        // Bersihkan sisa node TeamGdMod bila ada (mencegah node ganda)
        bersihkan(m_uiLayer, "tgm-hud-bg"_spr);
        bersihkan(m_uiLayer, "tgm-hud"_spr);
        bersihkan(m_uiLayer, "tgm-iv"_spr);
        bersihkan(m_uiLayer, "tgm-restart"_spr);
        bersihkan(m_uiLayer, "tgm-stepper"_spr);
        bersihkan(m_objectLayer, "tgm-jejak"_spr);
        m_fields->jejak1.clear();
        m_fields->jejak2.clear();
        m_fields->jejakNode = nullptr;

        // Label HUD + latar gelap
        auto latar = CCLayerColor::create(ccc4(0, 0, 0, 90));
        latar->setID("tgm-hud-bg"_spr);
        latar->setVisible(false);
        m_uiLayer->addChild(latar, 99);

        auto label = CCLabelBMFont::create("", "chatFont.fnt");
        label->setID("tgm-hud"_spr);
        m_fields->fontSaatIni = 0;
        terapkanGayaHud(label, latar, m_fields->fontSaatIni);
        m_uiLayer->addChild(label, 100);

        // Input Viewer
        m_fields->inputViewer = bangunInputViewer(this);
        m_uiLayer->addChild(m_fields->inputViewer, 102);   // di atas HUD agar tidak tertutup
        perbaruiInputViewer(m_fields->inputViewer, this, m_fields->ivCache);

        // Tombol restart kecil di layar (opsional)
        if (tgm::cfg().tombolRestart) {
            if (auto* spr = CCSprite::createWithSpriteFrameName("GJ_replayBtn_001.png")) {
                spr->setScale(0.45f);
                spr->setOpacity(160);
                auto* item = CCMenuItemExt::createSpriteExtra(spr, [this](auto) { this->resetLevelFromStart(); });
                item->setPosition({24.f, 24.f});
                auto* menu = CCMenu::create();
                menu->setID("tgm-restart"_spr);
                menu->setPosition({0.f, 0.f});
                menu->addChild(item);
                m_uiLayer->addChild(menu, 98);
            }
        }

        // Frame Stepper: tombol Jeda / Langkah di bawah layar
        tgm::stepper::reset();
        if (tgm::cfg().frameStepper) {
            auto win = CCDirector::get()->getWinSize();
            auto* menu = CCMenu::create();
            menu->setID("tgm-stepper"_spr);
            menu->setPosition({0.f, 0.f});

            auto* sprJeda = ButtonSprite::create("Jeda", "goldFont.fnt", "GJ_button_04.png", 0.8f);
            sprJeda->setScale(0.5f);
            auto* itemJeda = CCMenuItemExt::createSpriteExtra(sprJeda, [sprJeda](auto) {
                tgm::stepper::ubahBeku();
                sprJeda->setString(tgm::stepper::beku() ? "Lanjut" : "Jeda");
            });
            itemJeda->setPosition({win.width / 2.f - 42.f, 22.f});
            menu->addChild(itemJeda);

            auto* sprLangkah = ButtonSprite::create("Langkah", "goldFont.fnt", "GJ_button_04.png", 0.8f);
            sprLangkah->setScale(0.5f);
            auto* itemLangkah = CCMenuItemExt::createSpriteExtra(sprLangkah, [](auto) { tgm::stepper::langkah(); });
            itemLangkah->setPosition({win.width / 2.f + 42.f, 22.f});
            menu->addChild(itemLangkah);
            m_uiLayer->addChild(menu, 98);
        }

        // Hitbox: terapkan pengaturan debug draw GD
        m_fields->hitboxTerapkan = tgm::cfg().hitbox;
        if (tgm::cfg().hitbox || tgm::cfg().hitboxMati) this->updateDebugDrawSettings();

        // Presisi klik (CBF) dan musik practice
        tgm::cbf::levelDimuat();
        if (tgm::cfg().musikPracticeAsli) m_practiceMusicSync = true;

        // Jejak lintasan: node gambar di atas objek level
        if (m_objectLayer) {
            auto* node = CCDrawNode::create();
            node->setID("tgm-jejak"_spr);
            m_objectLayer->addChild(node, 1000);
            m_fields->jejakNode = node;
        }

        // Auto Practice: masuk mode Practice begitu level siap
        if (tgm::cfg().autoPractice && !m_isPracticeMode) {
            this->togglePracticeMode(true);
        }
    }

    // Hitbox memakai debug draw bawaan GD
    bool shouldDebugDraw() {
        if (tgm::cfg().hitbox) return true;
        return PlayLayer::shouldDebugDraw();
    }

    void updateDebugDrawSettings() {
        PlayLayer::updateDebugDrawSettings();
        auto const& c = tgm::cfg();
        if (c.hitbox) {
            m_isDebugDrawEnabled = true;
            if (m_debugDrawNode) m_debugDrawNode->setVisible(true);
            if (m_debugDrawNode2) m_debugDrawNode2->setVisible(true);
        }
        if (c.hitboxMati) m_hitboxesOnDeath = true;
    }

    // Musik practice sama dengan mode normal: pakai fitur Practice Music Sync bawaan GD
    // (musik level asli, sinkron dengan posisi checkpoint) alih-alih musik practice terpisah.
    void togglePracticeMode(bool praktik) {
        if (tgm::cfg().musikPracticeAsli) m_practiceMusicSync = true;
        PlayLayer::togglePracticeMode(praktik);
    }

    void toggleMusicInPractice() {
        if (tgm::cfg().musikPracticeAsli) m_practiceMusicSync = true;
        PlayLayer::toggleMusicInPractice();
    }

    void playPracticeMusic() {
        if (tgm::cfg().musikPracticeAsli) {
            // Jangan putar lagu practice terpisah: putar lagu level
            m_practiceMusicSync = true;
            PlayLayer::startMusic();
            return;
        }
        PlayLayer::playPracticeMusic();
    }

    void resetLevel() {
        if (tgm::cfg().musikPracticeAsli) m_practiceMusicSync = true;
        int percobaanSebelum = m_attempts;
        PlayLayer::resetLevel();

        // Bekukan Percobaan: nomor percobaan dikembalikan ke nilai sebelum reset
        if (tgm::cfg().bekukanPercobaan && m_attempts != percobaanSebelum) {
            m_attempts = percobaanSebelum;
            this->updateAttempts();
        }

        // Jejak lintasan dimulai lagi dari titik respawn
        m_fields->jejak1.clear();
        m_fields->jejak2.clear();
        if (m_fields->jejakNode) m_fields->jejakNode->clear();

        auto& st = tgm::stats();
        st.klik = 0;
        st.tabrakan = 0;
        st.frameTabrak = 0;
        st.frameTabrakAkhir = 0xFFFFFFFF;
        st.detikCp = 0.f;
        st.matiNoclip = 0;
        st.frameMatiNoclipAkhir = 0xFFFFFFFF;
        st.runMulai = this->getCurrentPercent();
        m_fields->menungguReset = false;
        m_fields->autoKillSudah = false;
        if (m_fields->musikDijeda) {
            FMODAudioEngine::get()->resumeMusic(0);
            m_fields->musikDijeda = false;
        }

        tgm::riwayat::catatPercobaan(idLevel(this));

        // Setelah respawn, sinkronkan bot dengan frame checkpoint
        auto frame = m_gameState.m_currentProgress;
        switch (tgm::bot::mode()) {
            case tgm::bot::Mode::Rekam: tgm::bot::potongDari(frame); break;
            case tgm::bot::Mode::Putar: tgm::bot::lompatKe(frame);   break;
            default:
                // Putar Otomatis: mulai dari awal level bila macro sudah dimuat
                if (tgm::cfg().autoPlay && frame == 0 && tgm::bot::jumlahAksi() > 0) {
                    tgm::bot::mulaiPutar();
                }
                break;
        }

        // Mode Aman diterapkan lagi (cheat bisa saja baru dinyalakan lewat menu)
        if (tgm::perluModeAman()) {
            m_isTestMode = true;
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto const& c = tgm::cfg();

        // Spike anti-cheat bawaan GD tetap harus diproses agar level tidak error
        if (c.noclip && object != m_anticheatSpike) {
            bool pemain1 = (player == m_player1);
            bool berlaku = pemain1 ? c.noclipP1 : c.noclipP2;
            if (berlaku) {
                auto& st = tgm::stats();
                auto frame = m_gameState.m_currentProgress;
                st.tabrakan++;
                auto frameU = static_cast<uint32_t>(frame);
                if (st.frameMatiNoclipAkhir == 0xFFFFFFFF || frameU > st.frameMatiNoclipAkhir + 1) st.matiNoclip++;
                st.frameMatiNoclipAkhir = frameU;
                if (frame != st.frameTabrakAkhir) {
                    st.frameTabrak++;
                    st.frameTabrakAkhir = frame;
                }
                // Mati bila melewati batas tabrakan atau akurasi turun di bawah ambang
                bool lewatBatas = (c.batasTabrakan > 0 && st.tabrakan > static_cast<uint32_t>(c.batasTabrakan));
                if (c.akurasiMin > 0 && frame > 120 && st.akurasi(frame) < static_cast<float>(c.akurasiMin)) {
                    lewatBatas = true;
                }
                if (!lewatBatas) return;
            }
        }

        int id = idLevel(this);
        tgm::sesi().mati++;
        tgm::riwayat::catatMati(id);
        int persenMati = static_cast<int>(this->getCurrentPercent());
        tgm::riwayat::catatTerbaik(id, persenMati);
        tgm::riwayat::catatZonaMati(id, persenMati);
        tgm::stats().persenMatiTerakhir = static_cast<float>(persenMati);

        // Best Run: run terpanjang (persen akhir - persen awal) pada sesi level ini
        {
            auto& st = tgm::stats();
            float akhir = this->getCurrentPercent();
            if (akhir - st.runMulai > st.bestRunB - st.bestRunA) {
                st.bestRunA = st.runMulai;
                st.bestRunB = akhir;
            }
        }

        PlayLayer::destroyPlayer(player, object);

        // Mute Audio Saat Mati: jeda musik level sampai respawn
        if (c.muteMati && !m_fields->musikDijeda) {
            FMODAudioEngine::get()->pauseMusic(0);
            m_fields->musikDijeda = true;
        }

        // Respawn cepat: jadwalkan reset lebih awal; panggilan reset yang terlambat diabaikan (lihat delayedResetLevel)
        if (c.respawnCepat && player && player->m_isDead) {
            m_fields->menungguReset = true;
            this->runAction(CCSequence::create(
                CCDelayTime::create(static_cast<float>(c.respawnMs) / 1000.f),
                CCCallFunc::create(this, callfunc_selector(TgmPlayLayer::delayedResetLevel)),
                nullptr
            ));
        }
    }

    void delayedResetLevel() {
        if (tgm::cfg().respawnCepat) {
            // Hanya satu reset per kematian: yang pertama (cepat atau bawaan) diproses, sisanya diabaikan
            if (!m_fields->menungguReset) return;
            m_fields->menungguReset = false;
        }
        PlayLayer::delayedResetLevel();
    }

    void levelComplete() {
        // Mode Aman: jangan simpan penyelesaian bila ada cheat aktif
        if (tgm::perluModeAman()) {
            m_isTestMode = true;
        }

        // Auto-simpan rekaman ke slot terpilih
        if (tgm::bot::mode() == tgm::bot::Mode::Rekam && tgm::cfg().autoSaveMacro) {
            std::string pesan;
            bool ok = tgm::bot::simpan(m_level->m_levelID.value(), tgm::cfg().slot, pesan);
            kabari(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
        }

        {
            auto& st = tgm::stats();
            if (100.f - st.runMulai > st.bestRunB - st.bestRunA) {
                st.bestRunA = st.runMulai;
                st.bestRunB = 100.f;
            }
        }

        int id = idLevel(this);
        tgm::riwayat::catatSelesai(id);
        tgm::riwayat::catatTerbaik(id, 100);
        tgm::db::flush();

        tgm::bot::berhenti();
        PlayLayer::levelComplete();
    }

    void onQuit() {
        tgm::bot::berhenti();
        tgm::stepper::reset();

        // Kembalikan Low Detail Mode ke nilai awal bila kita yang menyalakannya
        if (g_ldmDiubah) {
            GameManager::get()->setGameVariable("0042", g_ldmAsli);
            g_ldmDiubah = false;
        }

        // Catat sisa waktu main lalu simpan ke disk sebelum keluar level
        auto& st = tgm::stats();
        tgm::riwayat::catatDetik(idLevel(this), static_cast<int64_t>(st.detikBelumDicatat));
        st.detikBelumDicatat = 0.f;
        tgm::db::flush();

        PlayLayer::onQuit();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto const& c = tgm::cfg();
        auto& st = tgm::stats();

        // FPS terhaluskan (dt sudah diskalakan Speedhack, jadi dikembalikan ke waktu nyata)
        float dtNyata = dt / std::max(c.speed, 0.01f);
        if (dtNyata > 0.0001f) {
            st.fps = st.fps * 0.9f + (1.f / dtNyata) * 0.1f;
        }

        // Jam nyata untuk CPS; buang klik yang sudah lebih dari 1,5 detik
        st.waktuNyata += dtNyata;
        while (!st.waktuKlik.empty() && st.waktuNyata - st.waktuKlik.front() > 1.5f) st.waktuKlik.pop_front();

        // Waktu main permanen: dicatat per detik penuh; database di-flush tiap 10 detik
        st.detikBelumDicatat += dtNyata;
        if (st.detikBelumDicatat >= 1.f) {
            auto utuh = static_cast<int64_t>(st.detikBelumDicatat);
            st.detikBelumDicatat -= static_cast<float>(utuh);
            tgm::riwayat::catatDetik(idLevel(this), utuh);
        }
        m_fields->sejakFlush += dtNyata;
        if (m_fields->sejakFlush >= 10.f) {
            m_fields->sejakFlush = 0.f;
            tgm::db::flushJikaKotor();
        }

        // Checkpoint otomatis: hanya di mode practice, saat pemain hidup dan level belum selesai
        if (c.autoCheckpoint && m_isPracticeMode && !m_levelEndAnimationStarted
            && m_player1 && !m_player1->m_isDead) {
            st.detikCp += dtNyata;
            if (st.detikCp >= static_cast<float>(c.checkpointDtk)) {
                st.detikCp = 0.f;
                this->markCheckpoint();
            }
        }

        if (m_fields->inputViewer) perbaruiInputViewer(m_fields->inputViewer, this, m_fields->ivCache);

        // Auto Kill: matikan pemain begitu mencapai persen yang dipilih (sekali per percobaan)
        if (c.autoKill && !m_fields->autoKillSudah && !m_isPaused && !m_levelEndAnimationStarted
            && m_player1 && !m_player1->m_isDead
            && this->getCurrentPercent() >= static_cast<float>(c.autoKillPersen)) {
            m_fields->autoKillSudah = true;
            this->destroyPlayer(m_player1, m_anticheatSpike);
        }

        // Jejak lintasan: titik baru tiap 2 frame (4 frame di Mode Ringan)
        if (m_fields->jejakNode) {
            if (!c.jejak) {
                if (!m_fields->jejak1.empty() || !m_fields->jejak2.empty()) {
                    m_fields->jejak1.clear();
                    m_fields->jejak2.clear();
                    m_fields->jejakNode->clear();
                }
            } else if (m_player1 && !m_player1->m_isDead && !m_isPaused) {
                if (++m_fields->jejakAkum >= (c.modeRingan ? 4 : 2)) {
                    m_fields->jejakAkum = 0;
                    auto maks = static_cast<std::size_t>(c.modeRingan ? std::min(c.jejakPanjang, 120) : c.jejakPanjang);
                    m_fields->jejak1.push_back(m_player1->getPosition());
                    potongJejak(m_fields->jejak1, maks);
                    bool dua = m_gameState.m_isDualMode && m_player2;
                    if (dua) {
                        m_fields->jejak2.push_back(m_player2->getPosition());
                        potongJejak(m_fields->jejak2, maks);
                    } else {
                        m_fields->jejak2.clear();
                    }
                    gambarJejak(m_fields->jejakNode, m_fields->jejak1, m_fields->jejak2);
                }
            }
        }

        // Hitbox dinyalakan / dimatikan lewat menu saat level berjalan
        if (m_fields->hitboxTerapkan != c.hitbox) {
            m_fields->hitboxTerapkan = c.hitbox;
            this->updateDebugDrawSettings();
        }

        // Perbarui HUD paling banyak 10x per detik
        m_fields->akumulasiHud += dtNyata;
        if (m_fields->akumulasiHud < (c.modeRingan ? 0.25f : 0.1f)) return;
        m_fields->akumulasiHud = 0.f;

        auto* label = typeinfo_cast<CCLabelBMFont*>(m_uiLayer->getChildByID("tgm-hud"_spr));
        auto* latar = typeinfo_cast<CCLayerColor*>(m_uiLayer->getChildByID("tgm-hud-bg"_spr));
        if (!label) return;

        if (!c.showHud) {
            label->setVisible(false);
            if (latar) latar->setVisible(false);
            return;
        }
        label->setVisible(true);

        auto teks = susunTeksHud(this, m_isTestMode && tgm::perluModeAman());
        if (teks != m_fields->teksTerakhir) {
            m_fields->teksTerakhir = teks;
            label->setString(teks.c_str());
        }
        label->setVisible(!teks.empty());
        terapkanGayaHud(label, latar, m_fields->fontSaatIni);
    }
};

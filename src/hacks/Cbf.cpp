// Presisi klik untuk TeamGdMod.
//
// Cara kerja (disusun ulang untuk TeamGdMod, mengikuti pendekatan mod "Click Between Frames" oleh
// syzzi / theyareonit, lisensi MIT, lihat THIRD_PARTY.md):
//  - GD mencatat setiap sentuhan beserta stempel waktunya di m_queuedButtons.
//  - Di awal tiap frame fisika, antrean itu diambil dan dipetakan ke langkah fisika (240 per detik)
//    berdasarkan kapan sentuhan terjadi dalam frame.
//  - Mode 1 (Klik di Langkah): input diterapkan tepat di langkah yang sesuai.
//  - Mode 2 (CBF): langkah yang berisi input dipecah jadi sub-langkah, sehingga input diterapkan
//    pada saat yang tepat di antara dua langkah.
// Bila stempel waktu tidak cocok dengan jam frame (satuan / jam berbeda), fitur dimatikan sendiri
// untuk level itu supaya input tidak rusak.

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <time.h>
#include <vector>

#include "../bot/Bot.hpp"
#include "../core/Config.hpp"
#include "Cbf.hpp"

using namespace geode::prelude;

namespace {
    constexpr double TERKECIL = std::numeric_limits<float>::min();

    struct Langkah {
        PlayerButtonCommand input;
        double faktorDelta = 1.0;   // porsi langkah yang dijalankan sebelum input diterapkan
        bool   akhirLangkah = true; // true = langkah penuh (tanpa input di tengah)
    };

    PlayerButtonCommand inputKosong() {
        PlayerButtonCommand c{};
        c.m_button = PlayerButton::Jump;
        c.m_isPush = false;
        c.m_isPlayer2 = false;
        c.m_step = 0;
        c.m_timestamp = 0;
        return c;
    }

    std::vector<PlayerButtonCommand> g_inputVektor;
    std::deque<Langkah> g_antrean;
    PlayerButtonCommand g_inputBerikut = inputKosong();

    double g_waktuFrameLalu = 0.0;
    double g_waktuFrameKini = 0.0;
    bool   g_frameAwal = true;     // mencegah input nyasar saat level mulai / lanjut dari jeda
    bool   g_lewati = true;        // true = jangan pecah langkah pada PlayerObject::update()
    int    g_jumlahLangkah = 4;

    int    g_stempelBuruk = 0;     // frame berturut-turut dengan stempel waktu tidak cocok
    bool   g_rusak = false;        // fitur dimatikan untuk level ini

    // Pembantu pemecahan langkah di PlayerObject::update()
    CCPoint g_posP1 = {0.f, 0.f};
    CCPoint g_posP2 = {0.f, 0.f};
    float   g_deltaRotasi = 0.f;
    bool    g_inputLangkahIni = false;
    bool    g_pecahP1 = false;
    bool    g_pecahP2 = false;
    bool    g_tengahLangkah = false;

    double sekarang() {
        timespec t{};
        clock_gettime(CLOCK_MONOTONIC, &t);
        return static_cast<double>(t.tv_sec) + static_cast<double>(t.tv_nsec) / 1e9;
    }

    int modeDipilih() {
        return tgm::cfg().cbfMode;
    }

    void resetKeadaan() {
        g_frameAwal = true;
        g_lewati = true;
        g_inputVektor.clear();
        g_antrean.clear();
        g_inputBerikut = inputKosong();
    }

    // Susun antrean langkah dari input yang tercatat pada frame ini
    void bangunAntrean(int jumlahLangkah) {
        auto* pl = PlayLayer::get();
        g_inputBerikut = inputKosong();
        g_antrean.clear();
        g_lewati = false;

        if (g_frameAwal) {
            g_lewati = true;
            g_frameAwal = false;
            g_waktuFrameLalu = g_waktuFrameKini;
            g_inputVektor.clear();
            return;
        }

        // Validasi stempel waktu: harus berada di sekitar frame ini, kalau tidak (jam / satuan beda)
        // input dibiarkan diproses GD seperti biasa.
        for (auto const& in : pl->m_queuedButtons) {
            if (in.m_timestamp < g_waktuFrameLalu - 0.25 || in.m_timestamp > g_waktuFrameKini + 0.05) {
                g_lewati = true;
                if (++g_stempelBuruk >= 5 && !g_rusak) {
                    g_rusak = true;
                    Notification::create("Presisi klik dimatikan: stempel waktu input tidak cocok", NotificationIcon::Warning, 2.5f)->show();
                }
                return;
            }
        }
        g_stempelBuruk = 0;

        g_inputVektor.insert(g_inputVektor.end(), pl->m_queuedButtons.begin(), pl->m_queuedButtons.end());
        pl->m_queuedButtons.clear();

        double deltaWaktu = g_waktuFrameKini - g_waktuFrameLalu;
        double deltaLangkah = deltaWaktu / static_cast<double>(jumlahLangkah);

        std::size_t idx = 0;
        for (int i = 0; i < jumlahLangkah; ++i) {
            double terpakai = 0.0;
            while (idx < g_inputVektor.size()) {
                auto in = g_inputVektor[idx];
                if (in.m_timestamp - g_waktuFrameLalu < deltaLangkah * (i + 1)) {
                    double porsi = std::fmod(in.m_timestamp - g_waktuFrameLalu, deltaLangkah) / deltaLangkah;
                    if (!(porsi >= 0.0)) porsi = 0.0;   // juga menangkap NaN
                    g_antrean.push_back(Langkah{in, std::clamp(porsi - terpakai, TERKECIL, 1.0), false});
                    terpakai = porsi;
                    ++idx;
                } else {
                    break;
                }
            }
            g_antrean.push_back(Langkah{inputKosong(), std::max(TERKECIL, 1.0 - terpakai), true});
        }

        g_waktuFrameLalu = g_waktuFrameKini;
        g_inputVektor.erase(g_inputVektor.begin(), g_inputVektor.begin() + static_cast<std::ptrdiff_t>(idx));
    }

    // Ambil langkah terdepan; input dari langkah sebelumnya diterapkan di sini
    Langkah ambilLangkah() {
        if (g_antrean.empty()) return Langkah{inputKosong(), 1.0, true};

        Langkah depan = g_antrean.front();
        if (g_inputBerikut.m_timestamp != 0) {
            if (auto* pl = PlayLayer::get()) {
                pl->handleButton(g_inputBerikut.m_isPush, static_cast<int>(g_inputBerikut.m_button), !g_inputBerikut.m_isPlayer2);
            }
        }
        g_inputBerikut = depan.input;
        g_antrean.pop_front();
        return depan;
    }

    void kosongkanLog(PlayerObject* p) {
        p->m_collisionLogTop->removeAllObjects();
        p->m_collisionLogBottom->removeAllObjects();
        p->m_collisionLogLeft->removeAllObjects();
        p->m_collisionLogRight->removeAllObjects();
        p->m_lastCollisionLeft = -1;
        p->m_lastCollisionRight = -1;
        p->m_lastCollisionBottom = -1;
        p->m_lastCollisionTop = -1;
    }
}

namespace tgm::cbf {
    int modeEfektif() {
        if (g_rusak) return 0;
        if (!PlayLayer::get()) return 0;
        if (tgm::bot::mode() != tgm::bot::Mode::Siaga) return 0;
        return modeDipilih();
    }

    void awalFrame() {
        g_waktuFrameKini = sekarang();

        auto* pl = PlayLayer::get();
        CCNode* induk = nullptr;
        if (modeEfektif() == 0 || !pl || !(induk = pl->getParent())
            || induk->getChildByType<PauseLayer>(0)
            || pl->getChildByType<EndLevelLayer>(0)) {
            resetKeadaan();
        }
    }

    void sebelumProcessCommands() {
        // Mode 1: input diterapkan tepat di langkah yang sesuai (dikuantisasi ke 240 Hz)
        if (modeEfektif() == 1 && !g_antrean.empty()) {
            Langkah l;
            do l = ambilLangkah(); while (!g_antrean.empty() && !l.akhirLangkah);
        }
    }

    char const* status() {
        if (g_rusak) return "Dimatikan sendiri (stempel waktu tidak cocok)";
        switch (modeDipilih()) {
            case 1:  return "Klik di Langkah aktif";
            case 2:  return "Click Between Frames aktif";
            default: return "Mati (bawaan GD)";
        }
    }

    void levelDimuat() {
        g_rusak = false;
        g_stempelBuruk = 0;
        resetKeadaan();
        if (auto* pl = PlayLayer::get(); pl && modeDipilih() > 0) {
            // Matikan penanganan klik bawaan GD agar tidak bentrok
            pl->m_clickBetweenSteps = false;
            pl->m_clickOnSteps = false;
        }
    }
}

class $modify(TgmCbfLayer, GJBaseGameLayer) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("GJBaseGameLayer::getModifiedDelta", Priority::VeryEarly);
    }

    double getModifiedDelta(float delta) {
        double md = GJBaseGameLayer::getModifiedDelta(delta);

        auto* pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != this || tgm::cbf::modeEfektif() == 0) {
            g_lewati = true;
            return md;
        }

        float timewarp = pl->m_gameState.m_timeWarp;
        g_jumlahLangkah = static_cast<int>(std::round(std::max(1.0, ((md * 60.0) / std::min(1.0f, timewarp)) * 4.0)));

        if (pl->m_playerDied) {
            g_lewati = true;
            g_frameAwal = true;
        } else if (md > 0.0) {
            bangunAntrean(g_jumlahLangkah);
        } else {
            g_lewati = true;
        }
        return md;
    }
};

class $modify(TgmCbfPlayer, PlayerObject) {
    void update(float delta) {
        auto* pl = PlayLayer::get();

        // Lompat Bebas (eksperimental): dianggap selalu di tanah
        if (pl && tgm::cfg().lompatBebas) m_isOnGround = true;

        int mode = tgm::cbf::modeEfektif();
        if (!pl || mode != 2) {
            PlayerObject::update(delta);
            return;
        }

        // Seluruh logika dijalankan saat update P1 agar sederhana; P2 hanya diteruskan
        if ((this != pl->m_player1) || g_tengahLangkah) {
            if (g_tengahLangkah || !g_inputLangkahIni || this != pl->m_player2) PlayerObject::update(delta);
            return;
        }

        g_inputLangkahIni = g_antrean.empty() ? false : !g_antrean.front().akhirLangkah;
        if (!g_antrean.empty() && !g_inputLangkahIni) g_antrean.pop_front();

        if (g_lewati || !g_inputLangkahIni) {
            g_pecahP1 = false;
            g_pecahP2 = false;
            g_inputLangkahIni = false;
            PlayerObject::update(delta);
            return;
        }

        PlayerObject* p2 = pl->m_player2;
        bool dual = pl->m_gameState.m_isDualMode;
        bool p1DiTanah = this->m_isOnGround;
        bool p2DiTanah = p2->m_isOnGround;

        // Pemain yang sedang "buffering" (di udara tanpa ring) tidak dipecah agar tidak merusak fisika
        bool p1Bisa = p1DiTanah || this->m_touchingRings->count() || this->m_isDashing
            || (this->m_isDart || this->m_isBird || this->m_isShip || this->m_isSwing);
        bool p2Bisa = p2DiTanah || p2->m_touchingRings->count() || p2->m_isDashing
            || (p2->m_isDart || p2->m_isBird || p2->m_isShip || p2->m_isSwing);

        g_posP1 = PlayerObject::getPosition();
        g_posP2 = p2->getPosition();
        g_pecahP1 = p1Bisa;
        g_pecahP2 = p2Bisa && dual;

        Langkah l;
        bool pertama = true;
        g_tengahLangkah = true;

        do {
            l = ambilLangkah();
            float sub = delta * static_cast<float>(l.faktorDelta);
            g_deltaRotasi = sub;

            if (g_pecahP1) {
                PlayerObject::update(sub);
                if (!l.akhirLangkah) {
                    if (pertama && ((this->m_yVelocity < 0) ^ this->m_isUpsideDown)) this->m_isOnGround = p1DiTanah;
                    if (!this->m_isOnSlope || this->m_isDart) pl->checkCollisions(this, 0.0f, true);
                    else pl->checkCollisions(this, delta, true);
                    PlayerObject::updateRotation(sub);
                    kosongkanLog(this);
                }
            } else if (l.akhirLangkah) {
                PlayerObject::update(delta);
            }

            if (g_pecahP2) {
                p2->update(sub);
                if (!l.akhirLangkah) {
                    if (pertama && ((p2->m_yVelocity < 0) ^ p2->m_isUpsideDown)) p2->m_isOnGround = p2DiTanah;
                    if (!p2->m_isOnSlope || p2->m_isDart) pl->checkCollisions(p2, 0.0f, true);
                    else pl->checkCollisions(p2, delta, true);
                    p2->updateRotation(sub);
                    kosongkanLog(p2);
                }
            } else if (l.akhirLangkah) {
                p2->update(delta);
            }

            pertama = false;
        } while (!l.akhirLangkah);

        g_tengahLangkah = false;
    }

    void updateRotation(float t) {
        auto* pl = PlayLayer::get();
        if (pl && this == pl->m_player1 && g_pecahP1 && !g_tengahLangkah) {
            PlayerObject::updateRotation(g_deltaRotasi);   // sisa rotasi yang belum dikerjakan
            this->m_lastPosition = g_posP1;                // trigger gerak dan spider bisa bingung tanpa ini
        } else if (pl && this == pl->m_player2 && g_pecahP2 && !g_tengahLangkah) {
            PlayerObject::updateRotation(g_deltaRotasi);
            this->m_lastPosition = g_posP2;
        } else {
            PlayerObject::updateRotation(t);
        }
    }
};

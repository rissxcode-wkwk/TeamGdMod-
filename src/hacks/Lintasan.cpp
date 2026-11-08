// Tampilkan Lintasan: titik-titik perkiraan busur lompatan kubus (P1 dan P2).
// Ini PERKIRAAN dari kecepatan dan gravitasi pemain saat ini, bukan simulasi fisika penuh:
// tabrakan, portal, pad, dan wahana selain kubus tidak diperhitungkan.
// File ini berdiri sendiri: kalau build gagal gara-gara file ini, hapus saja.

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>

#include "../core/Config.hpp"

using namespace geode::prelude;

namespace {
    bool kubus(PlayerObject* p) {
        if (!p || p->m_isDead) return false;
        return !(p->m_isShip || p->m_isBird || p->m_isBall || p->m_isDart || p->m_isRobot || p->m_isSpider || p->m_isSwing);
    }

    void gambarBusur(CCDrawNode* node, PlayerObject* p, int langkah, ccColor4F warna) {
        constexpr double LOMPAT = 11.180032;    // kecepatan lompat kubus (satuan per frame 60 FPS)
        constexpr double GRAVITASI = 0.958199;  // percepatan jatuh per frame
        constexpr double JATUH_MAKS = 15.0;
        constexpr double KECEPATAN_X = 311.58;  // satuan per detik pada kecepatan 1x

        double arah = p->m_isUpsideDown ? -1.0 : 1.0;
        double v = p->m_isOnGround ? LOMPAT : p->m_yVelocity;   // di tanah: tampilkan lompatan bila klik sekarang
        double g = GRAVITASI * (p->m_gravityMod != 0.0 ? p->m_gravityMod : 1.0);
        double vx = KECEPATAN_X * static_cast<double>(p->m_playerSpeed) / 60.0 / 0.9;

        CCPoint pos = p->getPosition();
        double y0 = pos.y;
        for (int i = 0; i < langkah; ++i) {
            pos.x += static_cast<float>(vx);
            pos.y += static_cast<float>(arah * v);
            v = std::max(v - g, -JATUH_MAKS);
            if (i % 2 == 0) node->drawDot(pos, 1.6f, warna);
            // Kembali ke ketinggian awal saat jatuh: anggap mendarat
            if (v < 0.0 && arah * (pos.y - y0) <= 0.0) break;
        }
    }
}

class $modify(TgmLintasanLayer, PlayLayer) {
    struct Fields {
        CCDrawNode* node = nullptr;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto const& c = tgm::cfg();
        if (!c.lintasan) {
            if (m_fields->node) m_fields->node->clear();
            return;
        }
        if (!m_objectLayer) return;
        if (!m_fields->node) {
            auto* node = CCDrawNode::create();
            m_objectLayer->addChild(node, 1000);
            m_fields->node = node;
        }
        m_fields->node->clear();
        if (m_isPaused) return;

        int langkah = std::clamp(c.lintasanLangkah, 20, 120);
        if (kubus(m_player1)) gambarBusur(m_fields->node, m_player1, langkah, {0.3f, 1.f, 0.5f, 0.9f});
        bool dua = m_gameState.m_isDualMode || (m_levelSettings && m_levelSettings->m_twoPlayerMode);
        if (dua && kubus(m_player2)) gambarBusur(m_fields->node, m_player2, langkah, {1.f, 0.6f, 0.3f, 0.9f});
    }
};

#include "FloatingLogo.hpp"
#include "TeamPopup.hpp"

#include <Geode/ui/OverlayManager.hpp>

#include <algorithm>
#include <cmath>

#include "../core/Config.hpp"
#include "../core/Db.hpp"

using namespace geode::prelude;

namespace {
    FloatingLogo* g_inst = nullptr;
    bool g_pasangDijadwalkan = false;      // mencegah antrean pasang() ganda
    constexpr char const* ID_LOGO = "tgm-floating-logo";
    constexpr float RADIUS_DASAR = 22.f;     // jari-jari tombol pada ukuran 100%
    constexpr float AMBANG_GESER = 10.f;     // jarak minimal agar dianggap menggeser, bukan mengetuk
}

float FloatingLogo::jari() const {
    return RADIUS_DASAR * static_cast<float>(tgm::cfg().logoUkuran) / 100.f;
}

bool FloatingLogo::init() {
    if (!CCLayer::init()) return false;
    this->setID(ID_LOGO);
    this->setTouchMode(kCCTouchesOneByOne);
    this->setTouchEnabled(true);
    this->scheduleUpdate();
    this->bangunGambar();
    return true;
}

void FloatingLogo::bangunGambar() {
    if (m_gambar) m_gambar->removeFromParent();

    // Logo bulat dari resources; bila gagal dimuat, pakai tulisan "TG"
    CCNode* node = CCSprite::create("logo.png"_spr);
    if (!node) {
        auto l = CCLabelBMFont::create("TG", "goldFont.fnt");
        node = l;
    }
    m_gambar = node;
    this->addChild(m_gambar);
}

void FloatingLogo::registerWithTouchDispatcher() {
    // Prioritas sangat tinggi: tombol ini harus menerima sentuhan lebih dulu daripada popup dan level
    CCTouchDispatcher::get()->addTargetedDelegate(this, -9999, true);
}

void FloatingLogo::atasiBatas() {
    auto win = CCDirector::get()->getWinSize();
    float r = this->jari();
    m_x = std::clamp(m_x, r, win.width - r);
    m_y = std::clamp(m_y, r, win.height - r);
}

bool FloatingLogo::kena(CCPoint const& titik) const {
    float r = this->jari() + 6.f;   // sedikit lebih luas agar mudah disentuh
    float dx = titik.x - m_x, dy = titik.y - m_y;
    return dx * dx + dy * dy <= r * r;
}

void FloatingLogo::update(float dt) {
    auto const& c = tgm::cfg();
    auto win = CCDirector::get()->getWinSize();

    // Layer ini selalu menutupi layar penuh di titik (0,0); layout overlay Geode bisa menggesernya
    this->setContentSize(win);
    this->setPosition({0.f, 0.f});

    if (!m_posisiSiap) {
        m_x = m_tx = c.logoX * win.width;
        m_y = m_ty = c.logoY * win.height;
        this->atasiBatas();
        m_tx = m_x; m_ty = m_y;
        m_posisiSiap = true;
    }

    // Sembunyikan sesuai pengaturan
    bool dalamLevel = PlayLayer::get() != nullptr;
    bool tampil = c.logoTampil && (!dalamLevel || c.logoDiLevel);
    this->setVisible(tampil);
    this->setTouchEnabled(tampil);
    if (!tampil) { m_menekan = m_menyeret = false; return; }

    // Animasi menempel ke tepi
    if (!m_menyeret) {
        float k = std::min(1.f, dt * 14.f);
        m_x += (m_tx - m_x) * k;
        m_y += (m_ty - m_y) * k;
    }
    this->atasiBatas();

    // Ukuran, opasitas, dan redup otomatis
    m_diam += dt;
    float lebar = m_gambar->getContentSize().width;
    if (lebar < 1.f) lebar = 1.f;
    float skala = (this->jari() * 2.f) / lebar * (m_menekan ? 0.9f : 1.f);
    m_gambar->setScale(skala);
    m_gambar->setPosition({m_x, m_y});

    float opasitas = static_cast<float>(c.logoOpasitas) / 100.f;
    if (c.logoRedup && !m_menekan && m_diam > 3.f) opasitas = std::min(opasitas, 0.35f);
    if (auto* rgba = typeinfo_cast<CCRGBAProtocol*>(m_gambar)) {
        rgba->setOpacity(static_cast<GLubyte>(std::clamp(opasitas, 0.f, 1.f) * 255.f));
    }
}

bool FloatingLogo::ccTouchBegan(CCTouch* sentuh, CCEvent*) {
    if (!this->isVisible()) return false;
    auto titik = sentuh->getLocation();
    if (!this->kena(titik)) return false;
    m_menekan = true;
    m_menyeret = false;
    m_diam = 0.f;
    m_awalSentuh = titik;
    m_awalPosisi = ccp(m_x, m_y);
    return true;   // menelan sentuhan: level tidak ikut melompat
}

void FloatingLogo::ccTouchMoved(CCTouch* sentuh, CCEvent*) {
    if (!m_menekan) return;
    auto titik = sentuh->getLocation();
    auto geser = titik - m_awalSentuh;
    if (!m_menyeret && std::sqrt(geser.x * geser.x + geser.y * geser.y) > AMBANG_GESER) {
        m_menyeret = true;
    }
    if (m_menyeret) {
        m_x = m_awalPosisi.x + geser.x;
        m_y = m_awalPosisi.y + geser.y;
        this->atasiBatas();
        m_tx = m_x; m_ty = m_y;
        m_diam = 0.f;
    }
}

void FloatingLogo::ccTouchEnded(CCTouch*, CCEvent*) {
    bool menyeret = m_menyeret;
    m_menekan = false;
    m_menyeret = false;
    m_diam = 0.f;

    auto& c = tgm::cfg();
    auto win = CCDirector::get()->getWinSize();

    if (menyeret) {
        // Menempel ke tepi kiri / kanan terdekat bila Magnet aktif
        if (c.logoMagnet) {
            float r = this->jari();
            m_tx = (m_x < win.width / 2.f) ? r + 4.f : win.width - r - 4.f;
        } else {
            m_tx = m_x;
        }
        m_ty = m_y;
        c.logoX = m_tx / win.width;
        c.logoY = m_ty / win.height;
        tgm::saveConfig();   // posisi langsung masuk database
    } else {
        TeamPopup::bukaAtauTutup();
    }
}

void FloatingLogo::ccTouchCancelled(CCTouch*, CCEvent*) {
    m_menekan = false;
    m_menyeret = false;
}

void FloatingLogo::pasang() {
    g_pasangDijadwalkan = false;
    auto* overlay = OverlayManager::get();

    // Buang salinan lain bila ada (hanya g_inst yang boleh hidup)
    if (auto* anak = overlay->getChildren()) {
        std::vector<CCNode*> asing;
        for (auto* n : CCArrayExt<CCNode*>(anak)) {
            if (n != g_inst && n->getID() == ID_LOGO) asing.push_back(n);
        }
        for (auto* n : asing) n->removeFromParent();
    }

    if (!g_inst) {
        g_inst = new FloatingLogo();
        if (!g_inst->init()) {
            delete g_inst;
            g_inst = nullptr;
            return;
        }
        g_inst->autorelease();
        g_inst->retain();   // dipegang sendiri: overlay bisa mengosongkan anaknya saat tekstur dimuat ulang
    }
    // Tekstur bisa berubah (ganti kualitas grafis): bangun ulang gambar hanya bila sudah masuk overlay lagi
    if (g_inst->getParent() != overlay || !g_inst->m_gambar) g_inst->bangunGambar();
    if (g_inst->getParent() != overlay) {
        if (g_inst->getParent()) g_inst->removeFromParent();
        overlay->addChild(g_inst, 100000);
    }
}

void FloatingLogo::muatUlangGambar() {
    if (g_inst) g_inst->bangunGambar();
}

void FloatingLogo::resetPosisi() {
    auto& c = tgm::cfg();
    c.logoX = 0.94f;
    c.logoY = 0.62f;
    tgm::saveConfig();
    if (g_inst) g_inst->m_posisiSiap = false;
}

// Pasang saat mod selesai dimuat dan setiap kali tekstur dimuat ulang
// Satu pintu pemasangan: antrean ganda digabung jadi satu
static void jadwalkanPasang() {
    if (g_pasangDijadwalkan) return;
    g_pasangDijadwalkan = true;
    queueInMainThread([] { FloatingLogo::pasang(); });
}

$on_mod(Loaded) {
    jadwalkanPasang();
}

$on_game(TexturesLoaded) {
    // Tekstur dimuat ulang: gambar lama tidak valid lagi
    FloatingLogo::muatUlangGambar();
    jadwalkanPasang();
}

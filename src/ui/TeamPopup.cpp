#include "TeamPopup.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/Notification.hpp>
#include <algorithm>
#include <cmath>

#include "../bot/Bot.hpp"
#include "../core/Config.hpp"

using namespace geode::prelude;

namespace {
    // Tag untuk toggle
    constexpr int TAG_NOCLIP = 1;
    constexpr int TAG_AMAN   = 2;
    constexpr int TAG_HUD    = 3;

    void beriTahu(std::string const& pesan, NotificationIcon ikon = NotificationIcon::None) {
        Notification::create(pesan, ikon, 1.8f)->show();
    }

    // ID level yang sedang dimainkan, atau -1 bila tidak sedang di dalam level
    int idLevelSaatIni() {
        auto* pl = PlayLayer::get();
        if (!pl || !pl->m_level) return -1;
        return pl->m_level->m_levelID.value();
    }
}

TeamPopup* TeamPopup::create() {
    auto ret = new TeamPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool TeamPopup::init() {
    if (!Popup::init(340.f, 270.f, "square01_001.png")) return false;

    this->setTitle("TeamGdMod");

    auto& c = tgm::cfg();

    // --- Bagian hack ---
    this->tambahToggle("Noclip", TAG_NOCLIP, c.noclip, 82.f);
    this->tambahToggle("Mode Aman Otomatis", TAG_AMAN, c.autoSafeMode, 58.f);
    this->tambahToggle("Tampilkan HUD", TAG_HUD, c.showHud, 34.f);

    // Kecepatan: [-]  1.0x  [+]
    auto labelKec = CCLabelBMFont::create("Kecepatan", "bigFont.fnt");
    labelKec->setScale(0.45f);
    m_mainLayer->addChildAtPosition(labelKec, Anchor::Center, ccp(-112.f, 8.f), ccp(0.f, 0.5f));

    this->tambahTombol("-", menu_selector(TeamPopup::onKecepatan), 40.f, 8.f, 0.6f, -1);
    this->tambahTombol("+", menu_selector(TeamPopup::onKecepatan), 120.f, 8.f, 0.6f, 1);
    m_labelKecepatan = CCLabelBMFont::create("1.0x", "goldFont.fnt");
    m_labelKecepatan->setScale(0.6f);
    m_mainLayer->addChildAtPosition(m_labelKecepatan, Anchor::Center, ccp(80.f, 8.f));

    // --- Bagian Replay Bot ---
    auto judulBot = CCLabelBMFont::create("REPLAY BOT", "goldFont.fnt");
    judulBot->setScale(0.55f);
    m_mainLayer->addChildAtPosition(judulBot, Anchor::Center, ccp(0.f, -22.f));

    m_labelStatus = CCLabelBMFont::create("", "chatFont.fnt");
    m_labelStatus->setScale(0.7f);
    m_mainLayer->addChildAtPosition(m_labelStatus, Anchor::Center, ccp(0.f, -42.f));

    this->tambahTombol("Rekam", menu_selector(TeamPopup::onRekam), -100.f, -68.f);
    this->tambahTombol("Putar", menu_selector(TeamPopup::onPutar), 0.f, -68.f);
    this->tambahTombol("Stop",  menu_selector(TeamPopup::onBerhenti), 100.f, -68.f);

    // Slot: [<]  Slot 1  [>]   [Simpan] [Muat]
    this->tambahTombol("<", menu_selector(TeamPopup::onSlot), -135.f, -100.f, 0.55f, -1);
    this->tambahTombol(">", menu_selector(TeamPopup::onSlot), -45.f, -100.f, 0.55f, 1);
    m_labelSlot = CCLabelBMFont::create("Slot 1", "goldFont.fnt");
    m_labelSlot->setScale(0.5f);
    m_mainLayer->addChildAtPosition(m_labelSlot, Anchor::Center, ccp(-90.f, -100.f));
    this->tambahTombol("Simpan", menu_selector(TeamPopup::onSimpan), 30.f, -100.f, 0.55f);
    this->tambahTombol("Muat",   menu_selector(TeamPopup::onMuat), 110.f, -100.f, 0.55f);

    // Segarkan label status secara berkala
    this->schedule(schedule_selector(TeamPopup::segarkan), 0.2f);
    this->segarkan(0.f);

    return true;
}

void TeamPopup::tambahToggle(char const* teks, int tag, bool aktif, float y) {
    auto toggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(TeamPopup::onToggle), 0.65f
    );
    toggle->setTag(tag);
    toggle->toggle(aktif);
    m_buttonMenu->addChildAtPosition(toggle, Anchor::Center, ccp(-135.f, y));

    auto label = CCLabelBMFont::create(teks, "bigFont.fnt");
    label->setScale(0.45f);
    m_mainLayer->addChildAtPosition(label, Anchor::Center, ccp(-112.f, y), ccp(0.f, 0.5f));
}

void TeamPopup::tambahTombol(char const* teks, SEL_MenuHandler aksi, float x, float y, float skala, int tag) {
    auto spr = ButtonSprite::create(teks, "goldFont.fnt", "GJ_button_01.png", 0.8f);
    spr->setScale(skala);
    auto tombol = CCMenuItemSpriteExtra::create(spr, this, aksi);
    tombol->setTag(tag);
    m_buttonMenu->addChildAtPosition(tombol, Anchor::Center, ccp(x, y));
}

void TeamPopup::segarkan(float) {
    auto& c = tgm::cfg();

    m_labelKecepatan->setString(fmt::format("{:.1f}x", c.speed).c_str());
    m_labelSlot->setString(fmt::format("Slot {}", c.slot).c_str());

    char const* status = "Siaga";
    if (tgm::bot::mode() == tgm::bot::Mode::Rekam) status = "Merekam";
    if (tgm::bot::mode() == tgm::bot::Mode::Putar) status = "Memutar";
    m_labelStatus->setString(
        fmt::format("Status: {}  |  {} aksi", status, tgm::bot::jumlahAksi()).c_str()
    );
}

void TeamPopup::onToggle(CCObject* pengirim) {
    auto toggle = static_cast<CCMenuItemToggler*>(pengirim);
    // Callback dipanggil sebelum status toggle berubah, jadi nilai baru = kebalikannya
    bool aktif = !toggle->isToggled();

    auto& c = tgm::cfg();
    switch (toggle->getTag()) {
        case TAG_NOCLIP: c.noclip = aktif;       break;
        case TAG_AMAN:   c.autoSafeMode = aktif; break;
        case TAG_HUD:    c.showHud = aktif;      break;
        default: return;
    }
    tgm::saveConfig();
}

void TeamPopup::onKecepatan(CCObject* pengirim) {
    auto arah = static_cast<CCNode*>(pengirim)->getTag(); // -1 atau +1
    auto& c = tgm::cfg();
    c.speed = std::clamp(std::round((c.speed + 0.1f * arah) * 10.f) / 10.f, 0.1f, 3.f);
    tgm::saveConfig();
    this->segarkan(0.f);
}

void TeamPopup::onSlot(CCObject* pengirim) {
    auto arah = static_cast<CCNode*>(pengirim)->getTag();
    auto& c = tgm::cfg();
    c.slot = std::clamp(c.slot + arah, 1, 5);
    tgm::saveConfig();
    this->segarkan(0.f);
}

void TeamPopup::onRekam(CCObject*) {
    if (idLevelSaatIni() < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
    tgm::bot::mulaiRekam();
    beriTahu("Merekam: restart level lalu main", NotificationIcon::Success);
    this->segarkan(0.f);
}

void TeamPopup::onPutar(CCObject*) {
    if (idLevelSaatIni() < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
    if (tgm::bot::jumlahAksi() == 0) return beriTahu("Belum ada macro: rekam atau muat dulu", NotificationIcon::Warning);
    tgm::bot::mulaiPutar();
    beriTahu("Memutar: restart level untuk mulai dari awal", NotificationIcon::Success);
    this->segarkan(0.f);
}

void TeamPopup::onBerhenti(CCObject*) {
    tgm::bot::berhenti();
    beriTahu("Bot dihentikan");
    this->segarkan(0.f);
}

void TeamPopup::onSimpan(CCObject*) {
    auto id = idLevelSaatIni();
    if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
    std::string pesan;
    bool ok = tgm::bot::simpan(id, tgm::cfg().slot, pesan);
    beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
}

void TeamPopup::onMuat(CCObject*) {
    auto id = idLevelSaatIni();
    if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
    std::string pesan;
    bool ok = tgm::bot::muat(id, tgm::cfg().slot, pesan);
    beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
    this->segarkan(0.f);
}

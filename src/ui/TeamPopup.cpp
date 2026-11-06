#include "TeamPopup.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <cmath>
#include <string>

#include "../bot/Bot.hpp"
#include "../core/Config.hpp"
#include "../core/Stats.hpp"

using namespace geode::prelude;

namespace {
    // Tag toggle
    enum Toggle : int {
        T_NOCLIP = 1, T_AMAN, T_TANPA_EFEK, T_CP_OTOMATIS,
        T_HUD, T_FPS, T_KLIK, T_JAM,
        T_AUTO_SIMPAN, T_AUTO_PUTAR,
        T_P1, T_P2, T_PERSEN, T_PERCOBAAN, T_FRAME, T_WAKTU, T_MATI,
    };
    // Tag tombol +/-
    enum Step : int {
        S_KEC_MIN = 21, S_KEC_PLUS, S_CP_MIN, S_CP_PLUS,
        S_OPA_MIN, S_OPA_PLUS, S_SLOT_MIN, S_SLOT_PLUS,
        S_BATAS_MIN, S_BATAS_PLUS, S_UKURAN_MIN, S_UKURAN_PLUS,
        S_SUDUT_MIN, S_SUDUT_PLUS, S_WARNA_MIN, S_WARNA_PLUS,
    };
    enum Tab : int { TAB_HACK = 0, TAB_NOCLIP, TAB_HUD, TAB_GAYA, TAB_BOT, TAB_INFO };

    char const* NAMA_TAB[] = {"Hack", "Noclip", "HUD", "Gaya", "Bot", "Info"};
    char const* NAMA_SUDUT[] = {"Kiri Atas", "Kanan Atas", "Kiri Bawah", "Kanan Bawah"};
    char const* NAMA_WARNA[] = {"Putih", "Cyan", "Hijau", "Kuning", "Merah"};

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
    if (!Popup::init(380.f, 270.f, "square01_001.png")) return false;

    this->setTitle("TeamGdMod");

    auto& c = tgm::cfg();

    // ---------- Baris tab ----------
    for (int i = 0; i < JUMLAH_TAB; ++i) {
        auto spr = ButtonSprite::create(NAMA_TAB[i], "goldFont.fnt", "GJ_button_04.png", 0.8f);
        spr->setScale(0.5f);
        auto tombol = CCMenuItemSpriteExtra::create(spr, this, menu_selector(TeamPopup::onTab));
        tombol->setTag(i);
        m_buttonMenu->addChildAtPosition(tombol, Anchor::Center, ccp(-150.f + 60.f * i, 92.f));
        m_tombolTab[i] = tombol;
    }

    // ---------- Tab HACK ----------
    this->tambahToggle(TAB_HACK, "Noclip", T_NOCLIP, c.noclip, 56.f);
    this->tambahToggle(TAB_HACK, "Mode Aman Otomatis", T_AMAN, c.autoSafeMode, 30.f);
    this->tambahToggle(TAB_HACK, "Tanpa Efek Kematian", T_TANPA_EFEK, c.noDeathEffect, 4.f);
    this->tambahToggle(TAB_HACK, "Checkpoint Otomatis", T_CP_OTOMATIS, c.autoCheckpoint, -22.f);
    this->tambahStepper(TAB_HACK, S_CP_MIN, S_CP_PLUS, -22.f, m_lblCheckpoint);

    this->tambahLabel(TAB_HACK, "Kecepatan", -140.f, -52.f, 0.45f, "bigFont.fnt", true);
    this->tambahStepper(TAB_HACK, S_KEC_MIN, S_KEC_PLUS, -52.f, m_lblKecepatan);
    this->tambahTombol(TAB_HACK, "0.5x", menu_selector(TeamPopup::onPreset), -60.f, -78.f, 0.5f, 5);
    this->tambahTombol(TAB_HACK, "1x",   menu_selector(TeamPopup::onPreset), 0.f,   -78.f, 0.5f, 10);
    this->tambahTombol(TAB_HACK, "2x",   menu_selector(TeamPopup::onPreset), 60.f,  -78.f, 0.5f, 20);
    this->tambahLabel(TAB_HACK, "Cheat aktif = progres level tidak disimpan", 0.f, -102.f, 0.6f, "chatFont.fnt");

    // ---------- Tab NOCLIP ----------
    this->tambahToggle(TAB_NOCLIP, "Noclip", T_NOCLIP, c.noclip, 56.f);
    this->tambahToggle(TAB_NOCLIP, "Berlaku untuk Pemain 1", T_P1, c.noclipP1, 30.f);
    this->tambahToggle(TAB_NOCLIP, "Berlaku untuk Pemain 2", T_P2, c.noclipP2, 4.f);
    this->tambahLabel(TAB_NOCLIP, "Batas Tabrakan", -140.f, -24.f, 0.45f, "bigFont.fnt", true);
    this->tambahStepper(TAB_NOCLIP, S_BATAS_MIN, S_BATAS_PLUS, -24.f, m_lblBatas);
    this->tambahLabel(TAB_NOCLIP, "Bebas = tanpa batas. Bila terlampaui, pemain mati.", 0.f, -52.f, 0.6f, "chatFont.fnt");

    // ---------- Tab HUD ----------
    this->tambahToggle(TAB_HUD, "Tampilkan HUD", T_HUD, c.showHud, 62.f);
    this->tambahToggle(TAB_HUD, "FPS", T_FPS, c.showFps, 30.f);
    this->tambahToggle(TAB_HUD, "Klik", T_KLIK, c.showClicks, 4.f);
    this->tambahToggle(TAB_HUD, "Jam", T_JAM, c.showClock, -22.f);
    this->tambahToggle(TAB_HUD, "Persen", T_PERSEN, c.showPercent, -48.f);
    this->tambahToggle(TAB_HUD, "Percobaan", T_PERCOBAAN, c.showAttempt, 30.f, 10.f);
    this->tambahToggle(TAB_HUD, "Waktu", T_WAKTU, c.showTime, 4.f, 10.f);
    this->tambahToggle(TAB_HUD, "Frame", T_FRAME, c.showFrame, -22.f, 10.f);
    this->tambahToggle(TAB_HUD, "Mati Sesi", T_MATI, c.showDeaths, -48.f, 10.f);

    // ---------- Tab GAYA ----------
    this->tambahLabel(TAB_GAYA, "Kepekatan", -140.f, 52.f, 0.45f, "bigFont.fnt", true);
    this->tambahStepper(TAB_GAYA, S_OPA_MIN, S_OPA_PLUS, 52.f, m_lblOpasitas);
    this->tambahLabel(TAB_GAYA, "Ukuran", -140.f, 22.f, 0.45f, "bigFont.fnt", true);
    this->tambahStepper(TAB_GAYA, S_UKURAN_MIN, S_UKURAN_PLUS, 22.f, m_lblUkuran);
    this->tambahLabel(TAB_GAYA, "Posisi", -140.f, -8.f, 0.45f, "bigFont.fnt", true);
    this->tambahStepper(TAB_GAYA, S_SUDUT_MIN, S_SUDUT_PLUS, -8.f, m_lblSudut);
    this->tambahLabel(TAB_GAYA, "Warna", -140.f, -38.f, 0.45f, "bigFont.fnt", true);
    this->tambahStepper(TAB_GAYA, S_WARNA_MIN, S_WARNA_PLUS, -38.f, m_lblWarna);
    this->tambahLabel(TAB_GAYA, "Perubahan terlihat saat level dimainkan", 0.f, -72.f, 0.6f, "chatFont.fnt");

    // ---------- Tab BOT ----------
    m_lblStatus = this->tambahLabel(TAB_BOT, "", 0.f, 66.f, 0.7f, "chatFont.fnt");
    this->tambahTombol(TAB_BOT, "Rekam", menu_selector(TeamPopup::onRekam), -110.f, 40.f, 0.6f);
    this->tambahTombol(TAB_BOT, "Putar", menu_selector(TeamPopup::onPutar), 0.f, 40.f, 0.6f);
    this->tambahTombol(TAB_BOT, "Stop", menu_selector(TeamPopup::onBerhenti), 110.f, 40.f, 0.6f);

    this->tambahTombol(TAB_BOT, "<", menu_selector(TeamPopup::onStep), -150.f, 10.f, 0.5f, S_SLOT_MIN);
    m_lblSlot = this->tambahLabel(TAB_BOT, "Slot 1", -105.f, 10.f, 0.5f, "goldFont.fnt");
    this->tambahTombol(TAB_BOT, ">", menu_selector(TeamPopup::onStep), -60.f, 10.f, 0.5f, S_SLOT_PLUS);
    this->tambahTombol(TAB_BOT, "Simpan", menu_selector(TeamPopup::onSimpan), 10.f, 10.f, 0.5f);
    this->tambahTombol(TAB_BOT, "Muat",   menu_selector(TeamPopup::onMuat), 80.f, 10.f, 0.5f);
    this->tambahTombol(TAB_BOT, "Hapus",  menu_selector(TeamPopup::onHapus), 145.f, 10.f, 0.5f);

    m_lblSlotTerisi = this->tambahLabel(TAB_BOT, "", 0.f, -14.f, 0.6f, "chatFont.fnt");
    this->tambahToggle(TAB_BOT, "Simpan Otomatis Saat Selesai", T_AUTO_SIMPAN, c.autoSaveMacro, -40.f);
    this->tambahToggle(TAB_BOT, "Putar Otomatis dari Awal", T_AUTO_PUTAR, c.autoPlay, -66.f);
    this->tambahLabel(TAB_BOT, "Rekam: tekan Rekam, restart level, lalu main", 0.f, -96.f, 0.55f, "chatFont.fnt");

    // ---------- Tab INFO ----------
    this->tambahLabel(TAB_INFO, fmt::format("TeamGdMod {}", Mod::get()->getVersion().toVString()).c_str(), 0.f, 50.f, 0.7f, "goldFont.fnt");
    m_lblSesi = this->tambahLabel(TAB_INFO, "", 0.f, 26.f, 0.7f, "chatFont.fnt");
    this->tambahLabel(TAB_INFO, "Mode Aman mencegah progres tersimpan saat cheat aktif.", 0.f, 2.f, 0.6f, "chatFont.fnt");
    this->tambahLabel(TAB_INFO, "Jangan kirim rekor ke leaderboard memakai cheat.", 0.f, -16.f, 0.6f, "chatFont.fnt");
    this->tambahTombol(TAB_INFO, "Reset Pengaturan", menu_selector(TeamPopup::onReset), 0.f, -58.f, 0.6f);

    this->pilihTab(TAB_HACK);

    // Segarkan label dinamis secara berkala
    this->schedule(schedule_selector(TeamPopup::segarkan), 0.2f);
    this->segarkan(0.f);

    return true;
}

CCLabelBMFont* TeamPopup::tambahLabel(int tab, char const* teks, float x, float y, float skala, char const* font, bool rataKiri) {
    auto label = CCLabelBMFont::create(teks, font);
    label->setScale(skala);
    m_mainLayer->addChildAtPosition(label, Anchor::Center, ccp(x, y), ccp(rataKiri ? 0.f : 0.5f, 0.5f));
    m_isiTab[tab].push_back(label);
    return label;
}

void TeamPopup::tambahToggle(int tab, char const* teks, int tag, bool aktif, float y, float x) {
    auto toggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(TeamPopup::onToggle), 0.6f
    );
    toggle->setTag(tag);
    toggle->toggle(aktif);
    m_buttonMenu->addChildAtPosition(toggle, Anchor::Center, ccp(x, y));
    m_isiTab[tab].push_back(toggle);

    this->tambahLabel(tab, teks, x + 20.f, y, 0.42f, "bigFont.fnt", true);
}

CCMenuItemSpriteExtra* TeamPopup::tambahTombol(int tab, char const* teks, SEL_MenuHandler aksi, float x, float y, float skala, int tag) {
    auto spr = ButtonSprite::create(teks, "goldFont.fnt", "GJ_button_01.png", 0.8f);
    spr->setScale(skala);
    auto tombol = CCMenuItemSpriteExtra::create(spr, this, aksi);
    tombol->setTag(tag);
    m_buttonMenu->addChildAtPosition(tombol, Anchor::Center, ccp(x, y));
    m_isiTab[tab].push_back(tombol);
    return tombol;
}

void TeamPopup::tambahStepper(int tab, int tagMin, int tagPlus, float y, CCLabelBMFont*& label) {
    this->tambahTombol(tab, "-", menu_selector(TeamPopup::onStep), 85.f, y, 0.5f, tagMin);
    label = this->tambahLabel(tab, "", 125.f, y, 0.42f, "goldFont.fnt");
    this->tambahTombol(tab, "+", menu_selector(TeamPopup::onStep), 165.f, y, 0.5f, tagPlus);
}

void TeamPopup::pilihTab(int tab) {
    m_tabAktif = tab;
    for (int i = 0; i < JUMLAH_TAB; ++i) {
        bool aktif = (i == tab);
        for (auto* node : m_isiTab[i]) {
            node->setVisible(aktif);
        }
        // Tab aktif berwarna hijau, sisanya abu-abu
        auto* spr = static_cast<ButtonSprite*>(m_tombolTab[i]->getNormalImage());
        spr->updateBGImage(aktif ? "GJ_button_01.png" : "GJ_button_04.png");
    }
}

void TeamPopup::segarkan(float) {
    auto& c = tgm::cfg();

    m_lblKecepatan->setString(fmt::format("{:.1f}x", c.speed).c_str());
    m_lblCheckpoint->setString(fmt::format("{} dtk", c.checkpointDtk).c_str());
    m_lblOpasitas->setString(fmt::format("{}%", c.hudOpacity).c_str());
    m_lblUkuran->setString(fmt::format("{}%", c.hudScale).c_str());
    m_lblSudut->setString(NAMA_SUDUT[std::clamp(c.hudCorner, 0, 3)]);
    m_lblWarna->setString(NAMA_WARNA[std::clamp(c.hudColor, 0, 4)]);
    m_lblBatas->setString(c.batasTabrakan > 0 ? fmt::format("{}", c.batasTabrakan).c_str() : "Bebas");

    auto const& sesi = tgm::sesi();
    auto menit = static_cast<long long>(std::difftime(std::time(nullptr), sesi.mulai) / 60.0);
    m_lblSesi->setString(fmt::format("Sesi: {} menit | Mati: {} | Klik: {}", menit, sesi.mati, sesi.klik).c_str());
    m_lblSlot->setString(fmt::format("Slot {}", c.slot).c_str());

    char const* status = "Siaga";
    if (tgm::bot::mode() == tgm::bot::Mode::Rekam) status = "Merekam";
    if (tgm::bot::mode() == tgm::bot::Mode::Putar) status = "Memutar";
    m_lblStatus->setString(
        fmt::format("{} | {} aksi | {:.1f} dtk", status, tgm::bot::jumlahAksi(), tgm::bot::durasiDetik()).c_str()
    );

    // Daftar slot yang sudah berisi untuk level saat ini
    auto id = idLevelSaatIni();
    std::string terisi;
    if (id >= 0) {
        for (int s = 1; s <= 5; ++s) {
            if (tgm::bot::adaFile(id, s)) terisi += fmt::format("{} ", s);
        }
    }
    if (id < 0) {
        m_lblSlotTerisi->setString("Buka dari dalam level untuk memakai slot");
    } else {
        m_lblSlotTerisi->setString(fmt::format("Slot terisi: {}", terisi.empty() ? "-" : terisi).c_str());
    }
}

void TeamPopup::onTab(CCObject* pengirim) {
    this->pilihTab(static_cast<CCNode*>(pengirim)->getTag());
}

void TeamPopup::onToggle(CCObject* pengirim) {
    auto toggle = static_cast<CCMenuItemToggler*>(pengirim);
    // Callback dipanggil sebelum status toggle berubah, jadi nilai baru = kebalikannya
    bool aktif = !toggle->isToggled();

    auto& c = tgm::cfg();
    switch (toggle->getTag()) {
        case T_NOCLIP:      c.noclip = aktif;         break;
        case T_AMAN:        c.autoSafeMode = aktif;   break;
        case T_TANPA_EFEK:  c.noDeathEffect = aktif;  break;
        case T_CP_OTOMATIS: c.autoCheckpoint = aktif; break;
        case T_HUD:         c.showHud = aktif;        break;
        case T_FPS:         c.showFps = aktif;        break;
        case T_KLIK:        c.showClicks = aktif;     break;
        case T_JAM:         c.showClock = aktif;      break;
        case T_AUTO_SIMPAN: c.autoSaveMacro = aktif;  break;
        case T_AUTO_PUTAR:  c.autoPlay = aktif;       break;
        case T_P1:          c.noclipP1 = aktif;       break;
        case T_P2:          c.noclipP2 = aktif;       break;
        case T_PERSEN:      c.showPercent = aktif;    break;
        case T_PERCOBAAN:   c.showAttempt = aktif;    break;
        case T_FRAME:       c.showFrame = aktif;      break;
        case T_WAKTU:       c.showTime = aktif;       break;
        case T_MATI:        c.showDeaths = aktif;     break;
        default: return;
    }
    tgm::saveConfig();
}

void TeamPopup::onStep(CCObject* pengirim) {
    auto& c = tgm::cfg();
    switch (static_cast<CCNode*>(pengirim)->getTag()) {
        case S_KEC_MIN:  c.speed = std::round((c.speed - 0.1f) * 10.f) / 10.f; break;
        case S_KEC_PLUS: c.speed = std::round((c.speed + 0.1f) * 10.f) / 10.f; break;
        case S_CP_MIN:   c.checkpointDtk -= 1;  break;
        case S_CP_PLUS:  c.checkpointDtk += 1;  break;
        case S_OPA_MIN:  c.hudOpacity -= 10;    break;
        case S_OPA_PLUS: c.hudOpacity += 10;    break;
        case S_SLOT_MIN: c.slot -= 1;           break;
        case S_SLOT_PLUS:c.slot += 1;           break;
        case S_BATAS_MIN:  c.batasTabrakan -= 1;  break;
        case S_BATAS_PLUS: c.batasTabrakan += 1;  break;
        case S_UKURAN_MIN: c.hudScale -= 10;      break;
        case S_UKURAN_PLUS:c.hudScale += 10;      break;
        // Posisi dan warna berputar (setelah item terakhir kembali ke pertama)
        case S_SUDUT_MIN:  c.hudCorner = (c.hudCorner + 3) % 4; break;
        case S_SUDUT_PLUS: c.hudCorner = (c.hudCorner + 1) % 4; break;
        case S_WARNA_MIN:  c.hudColor = (c.hudColor + 4) % 5;   break;
        case S_WARNA_PLUS: c.hudColor = (c.hudColor + 1) % 5;   break;
        default: return;
    }
    c.speed = std::clamp(c.speed, 0.1f, 3.f);
    c.checkpointDtk = std::clamp(c.checkpointDtk, 1, 30);
    c.hudOpacity = std::clamp(c.hudOpacity, 10, 100);
    c.slot = std::clamp(c.slot, 1, 5);
    c.batasTabrakan = std::clamp(c.batasTabrakan, 0, 100);
    c.hudScale = std::clamp(c.hudScale, 50, 150);
    tgm::saveConfig();
    this->segarkan(0.f);
}

void TeamPopup::onPreset(CCObject* pengirim) {
    // Tag preset = kecepatan x 10 (5 -> 0.5x, 10 -> 1x, 20 -> 2x)
    tgm::cfg().speed = static_cast<float>(static_cast<CCNode*>(pengirim)->getTag()) / 10.f;
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
    this->segarkan(0.f);
}

void TeamPopup::onMuat(CCObject*) {
    auto id = idLevelSaatIni();
    if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
    std::string pesan;
    bool ok = tgm::bot::muat(id, tgm::cfg().slot, pesan);
    beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
    this->segarkan(0.f);
}

void TeamPopup::onHapus(CCObject*) {
    auto id = idLevelSaatIni();
    if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
    std::string pesan;
    bool ok = tgm::bot::hapus(id, tgm::cfg().slot, pesan);
    beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Warning);
    this->segarkan(0.f);
}

void TeamPopup::onReset(CCObject*) {
    tgm::resetConfig();
    beriTahu("Pengaturan direset. Buka menu lagi untuk melihat perubahan.", NotificationIcon::Success);
    this->onClose(nullptr);
}

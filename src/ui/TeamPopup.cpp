#include "TeamPopup.hpp"
#include "FloatingLogo.hpp"
#include "MegaMenu.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
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

using namespace geode::prelude;

namespace {
    char const* NAMA_TAB[] = {"Hack", "Noclip", "HUD", "Gaya HUD", "Input", "Bot", "Musik", "Tombol", "Statistik", "Data"};
    std::vector<std::string> const NAMA_SUDUT = {"Kiri Atas", "Kanan Atas", "Kiri Bawah", "Kanan Bawah", "Tengah Atas", "Tengah Bawah"};
    std::vector<std::string> const NAMA_WARNA = {"Putih", "Cyan", "Hijau", "Kuning", "Merah", "Oranye", "Ungu", "Pink"};
    std::vector<std::string> const NAMA_FONT  = {"Kecil (chat)", "Besar", "Emas"};
    std::vector<std::string> const NAMA_MODE_KLIK = {"Bawaan GD", "Klik di Langkah", "Click Between Frames"};

    void beriTahu(std::string const& pesan, NotificationIcon ikon = NotificationIcon::None) {
        Notification::create(pesan, ikon, 1.8f)->show();
    }

    // ID level yang sedang dimainkan, atau -1 bila tidak sedang di dalam level
    int idLevelSaatIni() {
        auto* pl = PlayLayer::get();
        if (!pl || !pl->m_level) return -1;
        return pl->m_level->m_levelID.value();
    }

    std::string formatDurasi(int64_t detik) {
        auto jam = detik / 3600;
        auto menit = (detik % 3600) / 60;
        if (jam > 0) return fmt::format("{} j {} m", jam, menit);
        return fmt::format("{} m {} d", menit, detik % 60);
    }

    constexpr int TAG_GANTI = 7701;   // tag aksi penjadwal ganti isi

    // Kumpulkan semua node yang punya opasitas (RGBA) beserta nilai aslinya, untuk animasi pudar
    void kumpulRgba(CCNode* node, std::vector<std::pair<CCNode*, GLubyte>>& hasil) {
        if (!node) return;
        if (auto* rgba = typeinfo_cast<CCRGBAProtocol*>(node)) hasil.emplace_back(node, rgba->getOpacity());
        if (auto* anak = node->getChildren()) {
            for (auto* n : CCArrayExt<CCNode*>(anak)) kumpulRgba(n, hasil);
        }
    }

    // Pudar masuk: dari 0 ke opasitas asli
    void pudarMasuk(CCNode* akar, float durasi) {
        std::vector<std::pair<CCNode*, GLubyte>> v;
        kumpulRgba(akar, v);
        for (auto& [n, asli] : v) {
            if (auto* rgba = typeinfo_cast<CCRGBAProtocol*>(n)) rgba->setOpacity(0);
            n->runAction(CCFadeTo::create(durasi, asli));
        }
    }

    // Pudar keluar: ke 0
    void pudarKeluar(CCNode* akar, float durasi) {
        std::vector<std::pair<CCNode*, GLubyte>> v;
        kumpulRgba(akar, v);
        for (auto& [n, asli] : v) n->runAction(CCFadeTo::create(durasi, 0));
    }

    // Respons tekan yang ringan: tombol hanya membesar sedikit saat ditekan
    void ringanSemua(CCNode* menu) {
        if (!menu || !menu->getChildren()) return;
        for (auto* n : CCArrayExt<CCNode*>(menu->getChildren())) {
            if (auto* item = typeinfo_cast<CCMenuItemSpriteExtra*>(n)) item->m_scaleMultiplier = 1.12f;
        }
    }

    // ---- Pembuat baris ----
    using Baris = TeamPopup::Baris;
    using Jenis = TeamPopup::Baris::Jenis;

    Baris judul(std::string teks) {
        Baris b; b.jenis = Jenis::Judul; b.teks = std::move(teks); return b;
    }

    Baris info(std::function<std::string()> fn) {
        Baris b; b.jenis = Jenis::Info; b.nilai = std::move(fn); return b;
    }

    Baris info(std::string teks) {
        return info([t = std::move(teks)] { return t; });
    }

    // Toggle yang langsung menyimpan ke database
    Baris tog(std::string teks, bool& ref) {
        Baris b; b.jenis = Jenis::Toggle; b.teks = std::move(teks);
        b.ambil = [&ref] { return ref; };
        b.setel = [&ref](bool v) { ref = v; tgm::saveConfig(); };
        return b;
    }

    Baris stepInt(std::string teks, int& ref, int mn, int mx, int langkah, std::function<std::string(int)> fmtNilai = nullptr) {
        Baris b; b.jenis = Jenis::Stepper; b.teks = std::move(teks);
        b.nilai = [&ref, fmtNilai] { return fmtNilai ? fmtNilai(ref) : std::to_string(ref); };
        b.ubah = [&ref, mn, mx, langkah](int arah) {
            ref = std::clamp(ref + arah * langkah, mn, mx);
            tgm::saveConfig();
        };
        return b;
    }

    Baris stepPilih(std::string teks, int& ref, std::vector<std::string> const& nama) {
        Baris b; b.jenis = Jenis::Stepper; b.teks = std::move(teks);
        int n = static_cast<int>(nama.size());
        b.nilai = [&ref, &nama, n] { return nama[std::clamp(ref, 0, n - 1)]; };
        b.ubah = [&ref, n](int arah) { ref = (ref + arah + n) % n; tgm::saveConfig(); };
        return b;
    }

    Baris stepKecepatan() {
        auto& c = tgm::cfg();
        Baris b; b.jenis = Jenis::Stepper; b.teks = "Kecepatan Game";
        b.nilai = [&c] { return fmt::format("{:.1f}x", c.speed); };
        b.ubah = [&c](int arah) {
            c.speed = std::clamp(std::round((c.speed + 0.1f * static_cast<float>(arah)) * 10.f) / 10.f, 0.1f, 3.f);
            tgm::saveConfig();
        };
        return b;
    }

    Baris tombol(std::vector<std::pair<std::string, std::function<void()>>> daftar) {
        Baris b; b.jenis = Jenis::Tombol; b.tombol = std::move(daftar); return b;
    }

    // Potong daftar baris menjadi halaman berisi BARIS_PER_HALAMAN baris
    std::vector<TeamPopup::Halaman> bagi(std::vector<Baris> semua, int perHalaman) {
        std::vector<TeamPopup::Halaman> hasil;
        for (std::size_t i = 0; i < semua.size(); i += static_cast<std::size_t>(perHalaman)) {
            auto akhir = std::min(semua.size(), i + static_cast<std::size_t>(perHalaman));
            hasil.emplace_back(semua.begin() + static_cast<std::ptrdiff_t>(i), semua.begin() + static_cast<std::ptrdiff_t>(akhir));
        }
        if (hasil.empty()) hasil.emplace_back();
        return hasil;
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

void TeamPopup::bukaAtauTutup() {
    auto* scene = CCDirector::get()->getRunningScene();
    if (!scene) return;

    // Menu jendela (gaya baru) menjadi bawaan; menu popup tab dipakai bila "Menu Klasik" dinyalakan
    if (MegaMenu::terbuka() || !tgm::cfg().menuKlasik) {
        MegaMenu::bukaAtauTutup();
        return;
    }

    // Sudah terbuka: tutup
    if (auto* ada = scene->getChildByType<TeamPopup>(0)) {
        ada->onClose(nullptr);
        return;
    }

    // Di dalam level yang sedang berjalan: jeda dulu agar pemain tidak mati saat menu dibuka
    if (auto* pl = PlayLayer::get(); pl && tgm::cfg().jedaSaatBuka && !pl->m_isPaused) {
        pl->pauseGame(false);
    }

    if (auto* popup = TeamPopup::create()) popup->show();
}

bool TeamPopup::init() {
    if (!Popup::init(440.f, 280.f, "square01_001.png")) return false;
    m_noElasticity = true;   // animasi buka diatur sendiri di show()
    this->setTitle(fmt::format("TeamGdMod  v{}", Mod::get()->getVersion().toVString(false)).c_str());

    // Layar sempit (mis. 4:3): kecilkan seluruh panel agar tidak terpotong
    auto win = CCDirector::get()->getWinSize();
    m_skala = std::min({1.f, (win.width - 16.f) / 440.f, (win.height - 8.f) / 280.f});
    m_mainLayer->setScale(m_skala);

    // Logo kecil di sisi kiri judul
    if (auto* logo = CCSprite::create("logo.png"_spr)) {
        logo->setScale(22.f / std::max(1.f, logo->getContentSize().width));
        m_mainLayer->addChildAtPosition(logo, Anchor::Center, ccp(-112.f, 122.f));
    }

    // Panel gelap tipis di belakang tab dan isi agar tampilan lebih terstruktur
    auto panel = [this](CCPoint pos, CCSize ukuran, int opasitas) {
        if (auto* p = NineSlice::create("square02b_001.png")) {
            p->setContentSize(ukuran);
            p->setColor({0, 0, 0});
            p->setOpacity(static_cast<GLubyte>(opasitas));
            m_mainLayer->addChildAtPosition(p, Anchor::Center, pos);
        }
    };
    panel({-172.f, -6.f}, {86.f, 252.f}, 70);
    panel({50.f, -10.f}, {330.f, 240.f}, 55);

    // Wadah isi halaman
    m_layerIsi = CCNode::create();
    m_mainLayer->addChild(m_layerIsi, 5);
    m_menuIsi = CCMenu::create();
    m_menuIsi->setPosition({0.f, 0.f});
    m_menuIsi->setContentSize(m_mainLayer->getContentSize());
    m_mainLayer->addChild(m_menuIsi, 6);

    // Tab di sisi kiri (vertikal)
    for (int i = 0; i < JUMLAH_TAB; ++i) {
        auto* spr = ButtonSprite::create(NAMA_TAB[i], 80, true, "goldFont.fnt", "GJ_button_04.png", 22.f, 0.55f);
        auto* item = CCMenuItemExt::createSpriteExtra(spr, [this, i](auto) { this->pilihTab(i); });
        m_buttonMenu->addChildAtPosition(item, Anchor::Center, ccp(-172.f, 104.f - 23.f * static_cast<float>(i)));
        m_tombolTab[i] = item;
    }

    // Navigasi halaman
    auto* kiri = ButtonSprite::create("<", "goldFont.fnt", "GJ_button_04.png", 0.8f);
    kiri->setScale(0.5f);
    m_panahKiri = CCMenuItemExt::createSpriteExtra(kiri, [this](auto) { this->gantiHalaman(-1); });
    auto* kanan = ButtonSprite::create(">", "goldFont.fnt", "GJ_button_04.png", 0.8f);
    kanan->setScale(0.5f);
    m_panahKanan = CCMenuItemExt::createSpriteExtra(kanan, [this](auto) { this->gantiHalaman(1); });
    m_buttonMenu->addChildAtPosition(m_panahKiri, Anchor::Center, ccp(35.f, -112.f));
    m_buttonMenu->addChildAtPosition(m_panahKanan, Anchor::Center, ccp(135.f, -112.f));
    m_lblHalaman = CCLabelBMFont::create("1/1", "goldFont.fnt");
    m_lblHalaman->setScale(0.45f);
    m_mainLayer->addChildAtPosition(m_lblHalaman, Anchor::Center, ccp(85.f, -112.f));

    ringanSemua(m_buttonMenu);
    this->pilihTab(std::clamp(tgm::cfg().tabTerakhir, 0, JUMLAH_TAB - 1), false);
    this->schedule(schedule_selector(TeamPopup::segarkan), 0.2f);
    return true;
}

void TeamPopup::pilihTab(int tab, bool animasi) {
    if (animasi && tab == m_tab) return;
    float arah = tab >= m_tab ? 1.f : -1.f;
    bool berubah = tab != tgm::cfg().tabTerakhir;
    m_tab = tab;
    if (berubah) {
        tgm::cfg().tabTerakhir = tab;
        tgm::saveConfig();
    }

    for (int i = 0; i < JUMLAH_TAB; ++i) {
        auto* spr = static_cast<ButtonSprite*>(m_tombolTab[i]->getNormalImage());
        spr->updateBGImage(i == tab ? "GJ_button_01.png" : "GJ_button_04.png");
    }

    auto bangun = [this] {
        m_halaman = bangunTab(m_tab, this);
        m_hal = 0;
        this->tampilkanHalaman();
    };
    if (animasi) this->ganti(arah, std::move(bangun));
    else bangun();
}

void TeamPopup::gantiHalaman(int arah) {
    int n = static_cast<int>(m_halaman.size());
    if (n <= 1) return;
    m_hal = (m_hal + arah + n) % n;
    this->ganti(static_cast<float>(arah), [this] { this->tampilkanHalaman(); });
}

// Ganti isi dengan animasi: isi lama pudar dan bergeser keluar, lalu isi baru pudar dan bergeser masuk.
// Konten tidak dibuat ulang sebelum animasi keluar selesai, jadi tidak ada kedipan.
void TeamPopup::ganti(float arah, std::function<void()> bangun) {
    if (m_menutup) return;
    m_tertunda = std::move(bangun);
    m_arah = arah;

    // Klik beruntun saat animasi berjalan: langsung selesaikan tanpa menumpuk aksi
    if (m_sibuk) {
        this->stopActionByTag(TAG_GANTI);
        this->selesaiKeluar();
        return;
    }
    m_sibuk = true;

    pudarKeluar(m_layerIsi, 0.07f);
    pudarKeluar(m_menuIsi, 0.07f);
    m_layerIsi->runAction(CCMoveBy::create(0.07f, {-8.f * arah, 0.f}));
    m_menuIsi->runAction(CCMoveBy::create(0.07f, {-8.f * arah, 0.f}));

    auto* urutan = CCSequence::create(
        CCDelayTime::create(0.07f),
        CCCallFunc::create(this, callfunc_selector(TeamPopup::selesaiKeluar)),
        nullptr);
    urutan->setTag(TAG_GANTI);
    this->runAction(urutan);
}

void TeamPopup::selesaiKeluar() {
    m_layerIsi->stopAllActions();
    m_menuIsi->stopAllActions();

    // Bangun isi baru, lalu masukkan dengan geser + pudar singkat
    if (m_tertunda) {
        auto fn = std::move(m_tertunda);
        m_tertunda = nullptr;
        fn();
    }
    m_layerIsi->setPosition({10.f * m_arah, 0.f});
    m_menuIsi->setPosition({10.f * m_arah, 0.f});
    m_layerIsi->runAction(CCEaseSineOut::create(CCMoveTo::create(0.12f, {0.f, 0.f})));
    m_menuIsi->runAction(CCEaseSineOut::create(CCMoveTo::create(0.12f, {0.f, 0.f})));
    pudarMasuk(m_layerIsi, 0.12f);
    pudarMasuk(m_menuIsi, 0.12f);
    m_sibuk = false;
}

// Animasi buka: pudar masuk + membesar halus
void TeamPopup::show() {
    Popup::show();
    m_mainLayer->setScale(m_skala * 0.9f);
    m_mainLayer->runAction(CCEaseSineOut::create(CCScaleTo::create(0.18f, m_skala)));
    pudarMasuk(m_mainLayer, 0.16f);
}

// Animasi tutup: pudar keluar + mengecil halus, baru dihapus dari scene
void TeamPopup::onClose(CCObject*) {
    if (m_menutup) return;
    m_menutup = true;
    this->unschedule(schedule_selector(TeamPopup::segarkan));
    this->stopActionByTag(TAG_GANTI);
    m_buttonMenu->setEnabled(false);
    m_menuIsi->setEnabled(false);

    pudarKeluar(m_mainLayer, 0.12f);
    m_mainLayer->runAction(CCEaseSineIn::create(CCScaleTo::create(0.12f, m_skala * 0.92f)));
    this->runAction(CCSequence::create(
        CCDelayTime::create(0.12f),
        CCCallFunc::create(this, callfunc_selector(TeamPopup::selesaiTutup)),
        nullptr));
}

void TeamPopup::selesaiTutup() {
    Popup::onClose(nullptr);
}

void TeamPopup::tampilkanHalaman() {
    m_layerIsi->removeAllChildren();
    m_menuIsi->removeAllChildren();
    m_dinamis.clear();

    m_hal = std::clamp(m_hal, 0, std::max(0, static_cast<int>(m_halaman.size()) - 1));
    bool banyak = m_halaman.size() > 1;
    m_panahKiri->setVisible(banyak);
    m_panahKanan->setVisible(banyak);
    m_lblHalaman->setVisible(banyak);
    m_lblHalaman->setString(fmt::format("{}/{}", m_hal + 1, m_halaman.size()).c_str());

    auto ukuran = m_mainLayer->getContentSize();
    CCPoint tengah = {ukuran.width / 2.f, ukuran.height / 2.f};
    constexpr float KIRI = -112.f, KANAN = 208.f, ATAS = 98.f, JARAK = 26.f;

    auto tambahLabel = [&](std::string const& teks, char const* font, float x, float y, float skala, float lebarMaks, bool kiriRata) {
        auto* l = CCLabelBMFont::create(teks.c_str(), font);
        l->setScale(skala);
        if (lebarMaks > 0.f) l->limitLabelWidth(lebarMaks, skala, 0.15f);
        l->setAnchorPoint({kiriRata ? 0.f : 0.5f, 0.5f});
        l->setPosition(tengah + ccp(x, y));
        m_layerIsi->addChild(l);
        return l;
    };

    auto& halaman = m_halaman[static_cast<std::size_t>(m_hal)];
    for (std::size_t i = 0; i < halaman.size(); ++i) {
        auto& b = halaman[i];
        float y = ATAS - JARAK * static_cast<float>(i);

        // Garis latar tipis agar baris mudah dibaca
        auto* latar = CCLayerColor::create(ccc4(0, 0, 0, i % 2 == 0 ? 38 : 20), KANAN - KIRI + 8.f, JARAK - 3.f);
        latar->setPosition(tengah + ccp(KIRI - 4.f, y - (JARAK - 3.f) / 2.f));
        if (b.jenis != Jenis::Judul) m_layerIsi->addChild(latar, -1);

        switch (b.jenis) {
            case Jenis::Judul:
                tambahLabel(b.teks, "goldFont.fnt", (KIRI + KANAN) / 2.f, y, 0.5f, 0.f, false);
                break;

            case Jenis::Info: {
                auto* l = tambahLabel(b.nilai(), "chatFont.fnt", (KIRI + KANAN) / 2.f, y, 0.62f, KANAN - KIRI - 6.f, false);
                m_dinamis.emplace_back(l, b.nilai);
                break;
            }

            case Jenis::Toggle: {
                auto* t = CCMenuItemExt::createTogglerWithStandardSprites(0.55f, [setel = b.setel](CCMenuItemToggler* tg) {
                    setel(!tg->isToggled());   // callback dipanggil sebelum status berubah
                });
                t->toggle(b.ambil());
                t->setPosition(tengah + ccp(KIRI + 14.f, y));
                m_menuIsi->addChild(t);
                tambahLabel(b.teks, "bigFont.fnt", KIRI + 34.f, y, 0.4f, KANAN - KIRI - 40.f, true);
                break;
            }

            case Jenis::Stepper: {
                tambahLabel(b.teks, "bigFont.fnt", KIRI + 6.f, y, 0.4f, 150.f, true);
                auto buatPanah = [&](char const* teks, float x, int arah) {
                    auto* spr = ButtonSprite::create(teks, "goldFont.fnt", "GJ_button_04.png", 0.8f);
                    spr->setScale(0.45f);
                    auto* item = CCMenuItemExt::createSpriteExtra(spr, [this, ubah = b.ubah, arah](auto) {
                        ubah(arah);
                        this->segarkan(0.f);
                    });
                    item->setPosition(tengah + ccp(x, y));
                    m_menuIsi->addChild(item);
                };
                buatPanah("-", KANAN - 112.f, -1);
                auto* nilai = tambahLabel(b.nilai(), "goldFont.fnt", KANAN - 62.f, y, 0.42f, 76.f, false);
                m_dinamis.emplace_back(nilai, b.nilai);
                buatPanah("+", KANAN - 12.f, 1);
                break;
            }

            case Jenis::Tombol: {
                std::size_t n = b.tombol.size();
                float lebar = (KANAN - KIRI) / static_cast<float>(n);
                for (std::size_t k = 0; k < n; ++k) {
                    auto* spr = ButtonSprite::create(b.tombol[k].first.c_str(), static_cast<int>(lebar * 1.35f), true, "goldFont.fnt", "GJ_button_01.png", 26.f, 0.6f);
                    spr->setScale(0.7f);
                    auto* item = CCMenuItemExt::createSpriteExtra(spr, [fn = b.tombol[k].second](auto) { fn(); });
                    item->setPosition(tengah + ccp(KIRI + lebar * (static_cast<float>(k) + 0.5f), y));
                    m_menuIsi->addChild(item);
                }
                break;
            }
        }
    }
    ringanSemua(m_menuIsi);
}

void TeamPopup::segarkan(float) {
    if (m_menutup) return;
    if (m_bangunUlang) {
        m_bangunUlang = false;
        m_halaman = bangunTab(m_tab, this);
        this->tampilkanHalaman();
        return;
    }
    for (auto& [label, fn] : m_dinamis) label->setString(fn().c_str());
}

std::vector<TeamPopup::Halaman> TeamPopup::bangunTab(int tab, TeamPopup* self) {
    auto& c = tgm::cfg();
    std::vector<Baris> r;
    int perHal = BARIS_PER_HALAMAN;

    switch (tab) {
        // ================= HACK =================
        case 0: {
            r.push_back(tog("Noclip", c.noclip));
            r.push_back(tog("Mode Aman Otomatis (cheat = progres tidak tersimpan)", c.autoSafeMode));
            r.push_back(tog("Tanpa Efek Kematian", c.noDeathEffect));
            r.push_back(stepKecepatan());
            r.push_back(tombol({
                {"0.5x", [&c] { c.speed = 0.5f; tgm::saveConfig(); }},
                {"1x",   [&c] { c.speed = 1.f;  tgm::saveConfig(); }},
                {"2x",   [&c] { c.speed = 2.f;  tgm::saveConfig(); }},
            }));
            r.push_back(tog("Respawn Cepat", c.respawnCepat));
            r.push_back(stepInt("Jeda Respawn", c.respawnMs, 0, 1000, 50, [](int v) { return fmt::format("{} ms", v); }));
            r.push_back(tog("Checkpoint Otomatis (mode practice)", c.autoCheckpoint));
            // halaman 2
            r.push_back(stepInt("Jeda Checkpoint", c.checkpointDtk, 1, 30, 1, [](int v) { return fmt::format("{} dtk", v); }));
            r.push_back(tog("Lompat Bebas (eksperimental)", c.lompatBebas));
            r.push_back(tog("Tanpa Guncangan Layar", c.tanpaGuncang));
            r.push_back(tog("Tanpa Portal Cermin", c.tanpaCermin));
            r.push_back(tog("Tampilkan Hitbox", c.hitbox));
            r.push_back(tog("Hitbox Saat Mati", c.hitboxMati));
            r.push_back(tog("Langkah Frame (tombol Jeda dan Langkah di layar)", c.frameStepper));
            r.push_back(tog("Buka Semua Ikon dan Warna (lokal)", c.bukaSemua));
            r.push_back(tog("Jeda Level Otomatis Saat Menu Dibuka", c.jedaSaatBuka));
            r.push_back(tog("Konfirmasi Sebelum Keluar Level", c.konfirmasiKeluar));
            r.push_back(tog("Tombol Restart di Layar (level berikutnya)", c.tombolRestart));
            r.push_back(info([] { return tgm::cheatsActive() ? std::string("Cheat aktif: progres level TIDAK disimpan") : std::string("Tidak ada cheat aktif"); }));
            break;
        }
        // ================= NOCLIP =================
        case 1: {
            r.push_back(tog("Noclip", c.noclip));
            r.push_back(tog("Berlaku untuk Pemain 1", c.noclipP1));
            r.push_back(tog("Berlaku untuk Pemain 2", c.noclipP2));
            r.push_back(stepInt("Batas Tabrakan", c.batasTabrakan, 0, 100, 1, [](int v) { return v > 0 ? std::to_string(v) : std::string("Bebas"); }));
            r.push_back(stepInt("Mati Bila Akurasi <", c.akurasiMin, 0, 99, 5, [](int v) { return v > 0 ? fmt::format("{}%", v) : std::string("Mati"); }));
            r.push_back(info([] {
                auto* pl = PlayLayer::get();
                if (!pl) return std::string("Akurasi dan tabrakan tampil di dalam level");
                auto const& s = tgm::stats();
                return fmt::format("Akurasi {:.1f}%  |  Tabrakan {}", s.akurasi(pl->m_gameState.m_currentProgress), s.tabrakan);
            }));
            r.push_back(info("Bila batas terlampaui, pemain mati seperti biasa."));
            break;
        }
        // ================= HUD =================
        case 2: {
            r.push_back(tog("Tampilkan HUD", c.showHud));
            r.push_back(tog("FPS", c.showFps));
            r.push_back(tog("CPS (klik per detik)", c.showCps));
            r.push_back(tog("Jumlah Klik", c.showClicks));
            r.push_back(tog("Jam", c.showClock));
            r.push_back(tog("Persentase", c.showPercent));
            r.push_back(tog("Nomor Percobaan", c.showAttempt));
            r.push_back(tog("Frame", c.showFrame));
            r.push_back(tog("Waktu Level", c.showTime));
            r.push_back(tog("Mati Selama Sesi", c.showDeaths));
            r.push_back(tog("Persen Terbaik", c.showBest));
            r.push_back(tog("Persen Mati Terakhir", c.showMatiTerakhir));
            r.push_back(tog("Nama Level", c.showLevelName));
            r.push_back(tog("Akurasi Noclip", c.showAkurasi));
            break;
        }
        // ================= GAYA HUD =================
        case 3: {
            r.push_back(stepInt("Kepekatan", c.hudOpacity, 10, 100, 10, [](int v) { return fmt::format("{}%", v); }));
            r.push_back(stepInt("Ukuran", c.hudScale, 50, 150, 10, [](int v) { return fmt::format("{}%", v); }));
            r.push_back(stepPilih("Posisi", c.hudCorner, NAMA_SUDUT));
            r.push_back(stepPilih("Warna", c.hudColor, NAMA_WARNA));
            r.push_back(stepPilih("Huruf", c.hudFont, NAMA_FONT));
            r.push_back(tog("Latar Gelap di Belakang Teks", c.hudPanel));
            r.push_back(stepInt("Geser Horizontal", c.hudOffX, -100, 100, 5));
            r.push_back(stepInt("Geser Vertikal", c.hudOffY, -100, 100, 5));
            break;
        }
        // ================= INPUT & KLIK =================
        case 4: {
            r.push_back(judul("Input Viewer"));
            r.push_back(tog("Tampilkan Input Viewer", c.ivTampil));
            r.push_back(stepPilih("Posisi", c.ivPosisi, NAMA_SUDUT));
            r.push_back(stepInt("Ukuran", c.ivUkuran, 50, 200, 10, [](int v) { return fmt::format("{}%", v); }));
            r.push_back(stepInt("Kepekatan", c.ivOpasitas, 20, 100, 10, [](int v) { return fmt::format("{}%", v); }));
            r.push_back(stepPilih("Warna Tombol", c.ivWarna, NAMA_WARNA));
            r.push_back(stepInt("Geser Horizontal", c.ivOffX, -150, 150, 5));
            r.push_back(stepInt("Geser Vertikal", c.ivOffY, -150, 150, 5));
            r.push_back(judul("Presisi Klik"));
            r.push_back(stepPilih("Mode Klik", c.cbfMode, NAMA_MODE_KLIK));
            r.push_back(info([] { return std::string(tgm::cbf::status()); }));
            r.push_back(tog("Klik Otomatis (tahan untuk spam)", c.turbo));
            r.push_back(stepInt("Kecepatan Klik Otomatis", c.turboCps, 5, 30, 1, [](int v) { return fmt::format("{} /dtk", v); }));
            r.push_back(info("Mode Klik aktif saat level dibuka ulang. Mati saat bot aktif."));
            break;
        }
        // ================= BOT =================
        case 5: {
            r.push_back(info([] {
                char const* s = "Siaga";
                if (tgm::bot::mode() == tgm::bot::Mode::Rekam) s = "Merekam";
                if (tgm::bot::mode() == tgm::bot::Mode::Putar) s = "Memutar";
                return fmt::format("{}  |  {} aksi  |  {:.1f} dtk", s, tgm::bot::jumlahAksi(), tgm::bot::durasiDetik());
            }));
            r.push_back(tombol({
                {"Rekam", [] {
                    if (idLevelSaatIni() < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
                    tgm::bot::mulaiRekam();
                    beriTahu("Merekam: restart level lalu main", NotificationIcon::Success);
                }},
                {"Putar", [] {
                    if (idLevelSaatIni() < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
                    if (tgm::bot::jumlahAksi() == 0) return beriTahu("Belum ada macro: rekam atau muat dulu", NotificationIcon::Warning);
                    tgm::bot::mulaiPutar();
                    beriTahu("Memutar: restart level untuk mulai dari awal", NotificationIcon::Success);
                }},
                {"Stop", [] { tgm::bot::berhenti(); beriTahu("Bot dihentikan"); }},
            }));
            r.push_back(stepInt("Slot Macro", c.slot, 1, 5, 1, [](int v) { return fmt::format("Slot {}", v); }));
            r.push_back(info([&c] {
                int id = idLevelSaatIni();
                if (id < 0) return std::string("Buka dari dalam level untuk memakai slot");
                auto i = tgm::bot::infoSlot(id, c.slot);
                if (!i.ada) return fmt::format("Slot {} kosong", c.slot);
                return fmt::format("Slot {}: {} aksi, {:.1f} dtk", c.slot, i.aksi, i.detik);
            }));
            r.push_back(tombol({
                {"Simpan", [&c] {
                    int id = idLevelSaatIni();
                    if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
                    std::string pesan;
                    bool ok = tgm::bot::simpan(id, c.slot, pesan);
                    beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
                }},
                {"Muat", [&c] {
                    int id = idLevelSaatIni();
                    if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
                    std::string pesan;
                    bool ok = tgm::bot::muat(id, c.slot, pesan);
                    beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
                }},
                {"Hapus", [&c] {
                    int id = idLevelSaatIni();
                    if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
                    std::string pesan;
                    bool ok = tgm::bot::hapus(id, c.slot, pesan);
                    beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
                }},
            }));
            r.push_back(tog("Simpan Otomatis Saat Level Selesai", c.autoSaveMacro));
            r.push_back(tog("Putar Otomatis dari Awal Level", c.autoPlay));
            r.push_back(tog("Muat Macro Otomatis Saat Level Dibuka", c.autoMuat));
            break;
        }
        // ================= MUSIK =================
        case 6: {
            r.push_back(judul("Musik Practice"));
            r.push_back(tog("Musik Practice Sama dengan Mode Normal", c.musikPracticeAsli));
            r.push_back(info([&c] {
                return c.musikPracticeAsli ? std::string("Practice memutar lagu level asli, sinkron dengan checkpoint")
                                           : std::string("Practice memakai musik practice bawaan GD");
            }));
            r.push_back(info("Biasanya GD memutar musik practice terpisah."));
            r.push_back(info("Dengan opsi ini lagu level tetap diputar seperti biasa,"));
            r.push_back(info("dan lanjut dari posisi checkpoint saat respawn."));
            r.push_back(info("Berlaku saat level dibuka atau practice dinyalakan."));
            break;
        }
        // ================= TOMBOL LOGO =================
        case 7: {
            r.push_back(tog("Tampilkan Tombol Logo", c.logoTampil));
            r.push_back(tog("Tampil Saat Bermain di Level", c.logoDiLevel));
            r.push_back(tog("Menempel ke Tepi Layar", c.logoMagnet));
            r.push_back(tog("Meredup Otomatis", c.logoRedup));
            r.push_back(stepInt("Ukuran", c.logoUkuran, 50, 200, 10, [](int v) { return fmt::format("{}%", v); }));
            r.push_back(stepInt("Kepekatan", c.logoOpasitas, 20, 100, 10, [](int v) { return fmt::format("{}%", v); }));
            r.push_back(tombol({{"Reset Posisi Tombol", [] { FloatingLogo::resetPosisi(); beriTahu("Posisi tombol dikembalikan"); }}}));
            r.push_back(info("Ketuk = buka menu. Tahan lalu geser = pindahkan."));
            break;
        }
        // ================= STATISTIK =================
        case 8: {
            r.push_back(judul("Keseluruhan"));
            r.push_back(info([] {
                auto t = tgm::riwayat::total();
                return fmt::format("Waktu main {}  |  Sesi {}", formatDurasi(t.detik), t.sesi);
            }));
            r.push_back(info([] {
                auto t = tgm::riwayat::total();
                return fmt::format("Mati {}  |  Klik {}", t.mati, t.klik);
            }));
            r.push_back(info([] {
                auto t = tgm::riwayat::total();
                return fmt::format("Percobaan {}  |  Selesai {}", t.percobaan, t.selesai);
            }));
            r.push_back(judul("Level Ini"));
            r.push_back(info([] {
                int id = idLevelSaatIni();
                if (id < 0) return std::string("Buka dari dalam level");
                auto l = tgm::riwayat::level(id);
                if (!l.ada) return std::string("Belum ada data");
                return fmt::format("Percobaan {}  |  Mati {}  |  Terbaik {}%", l.percobaan, l.mati, l.terbaik);
            }));
            r.push_back(info([] {
                int id = idLevelSaatIni();
                if (id < 0) return std::string("");
                auto l = tgm::riwayat::level(id);
                return fmt::format("Klik {}  |  Waktu {}  |  Selesai {}x", l.klik, formatDurasi(l.detik), l.selesai);
            }));
            r.push_back(info([] {
                int id = idLevelSaatIni();
                if (id < 0) return std::string("");
                auto z = tgm::riwayat::zonaTerbanyak(id);
                if (z.kotak < 0) return std::string("Belum ada data tempat mati");
                return fmt::format("Paling sering mati di {}-{}% ({}x)", z.kotak * 5, z.kotak * 5 + 5, z.jumlah);
            }));
            r.push_back(info([] {
                auto const& s = tgm::sesi();
                auto menit = static_cast<long long>(std::difftime(std::time(nullptr), s.mulai) / 60.0);
                return fmt::format("Sesi ini: {} menit  |  Mati {}  |  Klik {}", menit, s.mati, s.klik);
            }));
            r.push_back(info([] { return fmt::format("Level tercatat: {}", tgm::riwayat::jumlahLevelTercatat()); }));
            break;
        }
        // ================= DATA =================
        default: {
            r.push_back(judul("Database"));
            r.push_back(info([] { return "Status: " + tgm::db::status(); }));
            r.push_back(info("Disimpan setiap ada perubahan, aman walau game ditutup."));
            r.push_back(tombol({
                {"Cadangkan", [] {
                    bool ok = tgm::db::cadangkan();
                    beriTahu(ok ? "Cadangan dibuat" : "Gagal membuat cadangan", ok ? NotificationIcon::Success : NotificationIcon::Error);
                }},
                {"Pulihkan", [self] {
                    createQuickPopup("Pulihkan", "Ganti data sekarang dengan cadangan manual terakhir?", "Batal", "Pulihkan",
                        [self](auto*, bool ya) {
                            if (!ya) return;
                            if (tgm::db::pulihkanDariCadangan()) {
                                tgm::loadConfig();
                                beriTahu("Data dipulihkan", NotificationIcon::Success);
                                self->mintaBangunUlang();
                            } else {
                                beriTahu("Tidak ada cadangan manual", NotificationIcon::Warning);
                            }
                        });
                }},
            }));
            r.push_back(tombol({
                {"Hapus Statistik", [] {
                    createQuickPopup("Hapus Statistik", "Hapus semua statistik permanen? Pengaturan tidak ikut terhapus.", "Batal", "Hapus",
                        [](auto*, bool ya) {
                            if (!ya) return;
                            tgm::riwayat::hapusSemua();
                            beriTahu("Statistik dihapus");
                        });
                }},
                {"Reset Pengaturan", [self] {
                    createQuickPopup("Reset Pengaturan", "Kembalikan semua pengaturan ke bawaan?", "Batal", "Reset",
                        [self](auto*, bool ya) {
                            if (!ya) return;
                            tgm::resetConfig();
                            beriTahu("Pengaturan direset", NotificationIcon::Success);
                            self->mintaBangunUlang();
                        });
                }},
            }));
            r.push_back(judul("Peringatan"));
            r.push_back(info("Mode Aman mencegah progres tersimpan saat cheat aktif."));
            r.push_back(info("Jangan kirim rekor ke leaderboard memakai cheat."));
            break;
        }
    }
    return bagi(std::move(r), perHal);
}

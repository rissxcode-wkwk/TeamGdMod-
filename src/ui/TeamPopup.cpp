#include "TeamPopup.hpp"
#include "FloatingLogo.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/ScrollLayer.hpp>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <map>
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
    std::vector<std::string> const NAMA_SUDUT = {"Kiri Atas", "Kanan Atas", "Kiri Bawah", "Kanan Bawah", "Tengah Atas", "Tengah Bawah"};
    std::vector<std::string> const NAMA_WARNA = {"Putih", "Cyan", "Hijau", "Kuning", "Merah", "Oranye", "Ungu", "Pink"};
    std::vector<std::string> const NAMA_FONT  = {"Kecil (chat)", "Besar", "Emas"};
    std::vector<std::string> const NAMA_MODE_KLIK = {"Bawaan GD", "Klik di Langkah", "Click Between Frames"};
    std::vector<std::string> const NAMA_TEMA = {"Ungu", "Biru", "Hijau", "Merah", "Oranye", "Cyan", "Pink", "Emas"};

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

    // ---------------- Tema warna menu ----------------
    struct Tema {
        ccColor3B aksen, bg, barisAlt, kotak, dalam, teks, redup, tombol;
    };

    Tema ambilTema() {
        static ccColor3B const AKSEN[8] = {
            {168, 45, 198}, {52, 120, 246}, {46, 184, 98}, {220, 60, 60},
            {240, 140, 40}, {40, 190, 210}, {235, 90, 160}, {225, 180, 40},
        };
        auto const& c = tgm::cfg();
        Tema t{};
        t.aksen = AKSEN[std::clamp(c.tema, 0, 7)];
        if (c.menuTerang) {
            t.bg = {240, 240, 245};  t.barisAlt = {226, 226, 235};
            t.kotak = {110, 110, 125}; t.dalam = {255, 255, 255};
            t.teks = {28, 28, 36};   t.redup = {95, 95, 110};
            t.tombol = {212, 212, 224};
        } else {
            t.bg = {19, 19, 23};     t.barisAlt = {28, 28, 34};
            t.kotak = {80, 80, 92};  t.dalam = {36, 36, 44};
            t.teks = {236, 236, 242}; t.redup = {150, 150, 162};
            t.tombol = {42, 42, 52};
        }
        return t;
    }

    // Kotak berwarna. tengah = true: titik tengah di posisi (dipakai untuk isi tombol), false: pojok kiri bawah.
    CCLayerColor* kotak(float w, float h, ccColor3B c, int opas, bool tengah = false) {
        auto* k = CCLayerColor::create(ccc4(c.r, c.g, c.b, static_cast<GLubyte>(opas)), w, h);
        k->ignoreAnchorPointForPosition(false);
        k->setAnchorPoint(tengah ? ccp(0.5f, 0.5f) : ccp(0.f, 0.f));
        return k;
    }

    CCLabelBMFont* teks(std::string const& s, char const* font, float skala, float lebarMaks, ccColor3B warna, CCPoint anchor) {
        auto* l = CCLabelBMFont::create(s.c_str(), font);
        l->setScale(skala);
        if (lebarMaks > 0.f) l->limitLabelWidth(lebarMaks, skala, 0.1f);
        l->setColor(warna);
        l->setAnchorPoint(anchor);
        return l;
    }

    // ---------------- Pembuat baris ----------------
    using Baris = TeamPopup::Baris;
    using Jenis = TeamPopup::Baris::Jenis;

    Baris judul(std::string t) {
        Baris b; b.jenis = Jenis::Judul; b.teks = std::move(t); return b;
    }

    Baris info(std::function<std::string()> fn) {
        Baris b; b.jenis = Jenis::Info; b.nilai = std::move(fn); return b;
    }

    Baris info(std::string t) {
        return info([s = std::move(t)] { return s; });
    }

    // Toggle yang langsung menyimpan ke database. 'self' != nullptr: menu disusun ulang (mis. ganti tema).
    Baris tog(std::string t, bool& ref, TeamPopup* self = nullptr) {
        Baris b; b.jenis = Jenis::Toggle; b.teks = std::move(t);
        b.ambil = [&ref] { return ref; };
        b.setel = [&ref, self](bool v) {
            ref = v;
            tgm::saveConfig();
            if (self) self->mintaBangunUlang();
        };
        return b;
    }

    Baris stepInt(std::string t, int& ref, int mn, int mx, int langkah, std::function<std::string(int)> fmtNilai = nullptr, TeamPopup* self = nullptr) {
        Baris b; b.jenis = Jenis::Stepper; b.teks = std::move(t);
        b.nilai = [&ref, fmtNilai] { return fmtNilai ? fmtNilai(ref) : std::to_string(ref); };
        b.ubah = [&ref, mn, mx, langkah, self](int arah) {
            ref = std::clamp(ref + arah * langkah, mn, mx);
            tgm::saveConfig();
            if (self) self->mintaBangunUlang();
        };
        return b;
    }

    Baris stepPilih(std::string t, int& ref, std::vector<std::string> const& nama, TeamPopup* self = nullptr) {
        Baris b; b.jenis = Jenis::Stepper; b.teks = std::move(t);
        int n = static_cast<int>(nama.size());
        b.nilai = [&ref, &nama, n] { return nama[static_cast<std::size_t>(std::clamp(ref, 0, n - 1))]; };
        b.ubah = [&ref, n, self](int arah) {
            ref = (ref + arah + n) % n;
            tgm::saveConfig();
            if (self) self->mintaBangunUlang();
        };
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

    float tinggiBaris(Baris const& b) {
        switch (b.jenis) {
            case Jenis::Judul:   return 22.f;
            case Jenis::Toggle:  return 24.f;
            case Jenis::Stepper: return 42.f;
            case Jenis::Tombol:  return 30.f;
            default:             return 17.f;
        }
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
    auto win = CCDirector::get()->getWinSize();
    m_skala = std::clamp(static_cast<float>(tgm::cfg().menuSkala) / 100.f, 0.6f, 1.3f);
    float lebar = win.width / m_skala - 4.f;
    float tinggi = win.height / m_skala - 4.f;

    if (!Popup::init(lebar, tinggi, "square01_001.png")) return false;
    m_noElasticity = true;
    if (m_bgSprite) m_bgSprite->setVisible(false);   // latar diganti panel-panel
    if (m_closeBtn) m_closeBtn->setVisible(false);   // tombol tutup sendiri di bar atas
    m_mainLayer->setScale(m_skala);

    // Bar atas: logo + judul di kiri, navigasi halaman di tengah, tutup di kanan
    float x = 14.f;
    if (auto* logo = CCSprite::create("logo.png"_spr)) {
        logo->setScale(18.f / std::max(1.f, logo->getContentSize().width));
        m_mainLayer->addChildAtPosition(logo, Anchor::TopLeft, ccp(x, -15.f));
        x += 16.f;
    }
    auto* lblJudul = teks(fmt::format("TeamGdMod  v{}", Mod::get()->getVersion().toVString(false)),
                          "bigFont.fnt", 0.4f, 0.f, {255, 255, 255}, ccp(0.f, 0.5f));
    m_mainLayer->addChildAtPosition(lblJudul, Anchor::TopLeft, ccp(x, -15.f));

    auto buatPanah = [this](char const* t, float dx, int arah, CCLayerColor*& bg) {
        auto* node = kotak(26.f, 18.f, {168, 45, 198}, 255, true);
        bg = node;
        auto* l = teks(t, "bigFont.fnt", 0.4f, 0.f, {255, 255, 255}, ccp(0.5f, 0.5f));
        l->setPosition({13.f, 9.f});
        node->addChild(l);
        auto* item = CCMenuItemExt::createSpriteExtra(node, [this, arah](auto) { this->gantiHalaman(arah); });
        item->m_scaleMultiplier = 1.f;
        m_buttonMenu->addChildAtPosition(item, Anchor::Top, ccp(dx, -15.f));
        return item;
    };
    m_panahKiri = buatPanah("<", -52.f, -1, m_bgKiri);
    m_panahKanan = buatPanah(">", 52.f, 1, m_bgKanan);
    m_lblHalaman = teks("1/1", "bigFont.fnt", 0.36f, 0.f, {255, 255, 255}, ccp(0.5f, 0.5f));
    m_mainLayer->addChildAtPosition(m_lblHalaman, Anchor::Top, ccp(0.f, -15.f));

    if (auto* spr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png")) {
        spr->setScale(0.55f);
        auto* tutup = CCMenuItemExt::createSpriteExtra(spr, [this](auto) { this->onClose(nullptr); });
        m_buttonMenu->addChildAtPosition(tutup, Anchor::TopRight, ccp(-16.f, -15.f));
    }

    m_isi = CCNode::create();
    m_mainLayer->addChild(m_isi, 5);

    m_panel = bangunPanel(this);
    m_halaman = tgm::cfg().tabTerakhir;
    this->susun();
    this->schedule(schedule_selector(TeamPopup::segarkan), 0.15f);
    this->scheduleUpdate();
    return true;
}

void TeamPopup::gantiHalaman(int arah) {
    int jumlah = std::max(1, (static_cast<int>(m_panel.size()) + m_perHalaman - 1) / m_perHalaman);
    if (jumlah <= 1) return;
    m_halaman = (m_halaman + arah + jumlah) % jumlah;
    tgm::cfg().tabTerakhir = m_halaman;
    tgm::saveConfig();
    this->susun();
}

// Bangun ulang semua panel pada halaman aktif. Posisi scroll tiap panel dipertahankan.
void TeamPopup::susun() {
    auto& c = tgm::cfg();
    auto tema = ambilTema();

    // Simpan posisi scroll (jarak dari atas) sebelum dibongkar
    std::map<int, float> simpan;
    for (auto& s : m_skrol) {
        simpan[s.panel] = s.layer->m_contentLayer->getPositionY() - (s.tinggiLihat - s.tinggiIsi);
    }

    m_isi->removeAllChildren();
    m_skrol.clear();
    m_centang.clear();
    m_dinamis.clear();

    auto ukuran = m_mainLayer->getContentSize();
    constexpr float BAR = 30.f, TEPI = 8.f, JARAK = 6.f, HEADER = 22.f, LEBAR_MIN = 150.f;
    float areaAtas = ukuran.height - BAR - 2.f;
    float areaTinggi = areaAtas - TEPI;

    int n = static_cast<int>(m_panel.size());
    m_perHalaman = std::clamp(static_cast<int>((ukuran.width - 2.f * TEPI + JARAK) / (LEBAR_MIN + JARAK)), 1, std::max(1, n));
    int jumlahHal = std::max(1, (n + m_perHalaman - 1) / m_perHalaman);
    m_halaman = std::clamp(m_halaman, 0, jumlahHal - 1);
    float lebar = (ukuran.width - 2.f * TEPI - JARAK * static_cast<float>(m_perHalaman - 1)) / static_cast<float>(m_perHalaman);

    // Bar atas: navigasi halaman hanya muncul bila ada lebih dari satu halaman
    bool banyak = jumlahHal > 1;
    m_panahKiri->setVisible(banyak);
    m_panahKanan->setVisible(banyak);
    m_lblHalaman->setVisible(banyak);
    m_lblHalaman->setString(fmt::format("{}/{}", m_halaman + 1, jumlahHal).c_str());
    if (m_bgKiri) m_bgKiri->setColor(tema.aksen);
    if (m_bgKanan) m_bgKanan->setColor(tema.aksen);

    auto* menuHeader = CCMenu::create();
    menuHeader->setPosition({0.f, 0.f});
    menuHeader->setContentSize(ukuran);
    m_isi->addChild(menuHeader, 10);

    GLubyte opasPanel = static_cast<GLubyte>(std::clamp(c.menuOpasitas, 40, 100) * 255 / 100);

    int awal = m_halaman * m_perHalaman;
    for (int k = 0; k < m_perHalaman && awal + k < n; ++k) {
        int idx = awal + k;
        auto& panel = m_panel[static_cast<std::size_t>(idx)];
        bool lipat = (c.panelTutup >> idx) & 1;
        float x = TEPI + static_cast<float>(k) * (lebar + JARAK);

        // Tinggi isi seluruh baris
        float tinggiIsi = 6.f;
        for (auto const& b : panel.baris) tinggiIsi += tinggiBaris(b);
        float tinggiLihat = lipat ? 0.f : std::min(tinggiIsi, areaTinggi - HEADER);
        float tinggiPanel = HEADER + tinggiLihat;
        float bawah = areaAtas - tinggiPanel;

        // Latar panel + header
        auto* latar = kotak(lebar, tinggiPanel, tema.bg, opasPanel);
        latar->setPosition({x, bawah});
        m_isi->addChild(latar, 0);
        auto* head = kotak(lebar, HEADER, tema.aksen, 255);
        head->setPosition({x, areaAtas - HEADER});
        m_isi->addChild(head, 1);

        // Segitiga lipat + judul panel
        auto* segitiga = CCDrawNode::create();
        float cx = x + 11.f, cy = areaAtas - HEADER / 2.f;
        CCPoint titik[3];
        if (lipat) { titik[0] = ccp(cx - 2.f, cy + 4.f); titik[1] = ccp(cx - 2.f, cy - 4.f); titik[2] = ccp(cx + 3.f, cy); }
        else       { titik[0] = ccp(cx - 4.f, cy + 2.f); titik[1] = ccp(cx + 4.f, cy + 2.f); titik[2] = ccp(cx, cy - 3.f); }
        segitiga->drawPolygon(titik, 3, {1.f, 1.f, 1.f, 1.f}, 0.f, {0.f, 0.f, 0.f, 0.f});
        m_isi->addChild(segitiga, 2);
        auto* lblNama = teks(panel.nama, "bigFont.fnt", 0.36f, lebar - 40.f, {255, 255, 255}, ccp(0.5f, 0.5f));
        lblNama->setPosition({x + lebar / 2.f, cy});
        m_isi->addChild(lblNama, 2);

        auto* kena = kotak(lebar, HEADER, {0, 0, 0}, 0, true);
        auto* itemHead = CCMenuItemExt::createSpriteExtra(kena, [this, idx](auto) {
            tgm::cfg().panelTutup ^= (1 << idx);
            tgm::saveConfig();
            this->mintaBangunUlang();   // dibangun ulang di siklus berikutnya (aman dari callback)
        });
        itemHead->m_scaleMultiplier = 1.f;
        itemHead->setPosition({x + lebar / 2.f, cy});
        menuHeader->addChild(itemHead);

        if (lipat) continue;

        // Isi panel: daftar yang bisa di-scroll
        auto* skrol = ScrollLayer::create({lebar, tinggiLihat});
        skrol->setPosition({x, bawah});
        auto* konten = skrol->m_contentLayer;
        float tinggiKonten = std::max(tinggiIsi, tinggiLihat);
        konten->setContentSize({lebar, tinggiKonten});

        auto* menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        menu->setContentSize({lebar, tinggiKonten});
        konten->addChild(menu, 3);

        float y = tinggiKonten - 3.f;   // tepi atas baris berikutnya
        int urut = 0;
        for (auto const& b : panel.baris) {
            float h = tinggiBaris(b);
            float tengahY = y - h / 2.f;

            if (b.jenis != Jenis::Judul && urut % 2 == 1) {
                auto* alt = kotak(lebar, h, tema.barisAlt, 255);
                alt->setPosition({0.f, y - h});
                konten->addChild(alt, 0);
            }

            switch (b.jenis) {
                case Jenis::Judul: {
                    auto* l = teks(b.teks, "bigFont.fnt", 0.32f, lebar - 16.f, tema.aksen, ccp(0.f, 0.5f));
                    l->setPosition({8.f, tengahY});
                    konten->addChild(l, 1);
                    auto* garis = kotak(lebar - 16.f, 1.f, tema.aksen, 150);
                    garis->setPosition({8.f, y - h + 2.f});
                    konten->addChild(garis, 1);
                    break;
                }

                case Jenis::Info: {
                    auto* l = teks(b.nilai(), "chatFont.fnt", 0.5f, lebar - 14.f, tema.redup, ccp(0.f, 0.5f));
                    l->setPosition({8.f, tengahY});
                    konten->addChild(l, 1);
                    m_dinamis.emplace_back(l, b.nilai);
                    break;
                }

                case Jenis::Toggle: {
                    auto* luar = kotak(14.f, 14.f, tema.kotak, 255);
                    luar->setPosition({8.f, tengahY - 7.f});
                    konten->addChild(luar, 1);
                    auto* dalam = kotak(10.f, 10.f, tema.dalam, 255);
                    dalam->setPosition({10.f, tengahY - 5.f});
                    dalam->setColor(b.ambil() ? tema.aksen : tema.dalam);
                    konten->addChild(dalam, 2);
                    m_centang.push_back({dalam, b.ambil, tema.aksen, tema.dalam});

                    auto* l = teks(b.teks, "chatFont.fnt", 0.6f, lebar - 34.f, tema.teks, ccp(0.f, 0.5f));
                    l->setPosition({28.f, tengahY});
                    konten->addChild(l, 1);

                    auto* hit = kotak(lebar - 2.f, h - 2.f, {0, 0, 0}, 0, true);
                    auto* item = CCMenuItemExt::createSpriteExtra(hit, [this, ambil = b.ambil, setel = b.setel](auto) {
                        setel(!ambil());
                        this->segarkanCentang();
                    });
                    item->m_scaleMultiplier = 1.f;
                    item->setPosition({lebar / 2.f, tengahY});
                    menu->addChild(item);
                    break;
                }

                case Jenis::Stepper: {
                    // Dua baris: nama di atas, kontrol [-] nilai [+] di bawah selebar panel (mudah ditekan di HP)
                    auto* l = teks(b.teks, "chatFont.fnt", 0.6f, lebar - 14.f, tema.teks, ccp(0.f, 0.5f));
                    l->setPosition({8.f, y - 11.f});
                    konten->addChild(l, 1);

                    float kontrolY = y - 29.f;
                    float lebarNilai = lebar - 2.f * 8.f - 2.f * 26.f - 2.f * 4.f;
                    auto* kotakNilai = kotak(lebarNilai, 20.f, tema.dalam, 255, true);
                    kotakNilai->setPosition({lebar / 2.f, kontrolY});
                    konten->addChild(kotakNilai, 1);
                    auto* lblNilai = teks(b.nilai(), "chatFont.fnt", 0.6f, lebarNilai - 6.f, tema.teks, ccp(0.5f, 0.5f));
                    lblNilai->setPosition({lebar / 2.f, kontrolY});
                    konten->addChild(lblNilai, 2);
                    m_dinamis.emplace_back(lblNilai, b.nilai);

                    auto buatTombol = [&](char const* simbol, float tx, int arah) {
                        auto* node = kotak(26.f, 20.f, tema.tombol, 255, true);
                        auto* ls = teks(simbol, "bigFont.fnt", 0.42f, 0.f, tema.teks, ccp(0.5f, 0.5f));
                        ls->setPosition({13.f, 10.f});
                        node->addChild(ls);
                        auto* item = CCMenuItemExt::createSpriteExtra(node, [this, ubah = b.ubah, arah](auto) {
                            ubah(arah);
                            this->segarkan(0.f);
                        });
                        item->m_scaleMultiplier = 1.f;
                        item->setPosition({tx, kontrolY});
                        menu->addChild(item);
                    };
                    buatTombol("-", 8.f + 13.f, -1);
                    buatTombol("+", lebar - 8.f - 13.f, 1);
                    break;
                }

                case Jenis::Tombol: {
                    std::size_t jumlah = b.tombol.size();
                    float lebarTombol = (lebar - 8.f - 4.f * static_cast<float>(jumlah - 1)) / static_cast<float>(jumlah);
                    for (std::size_t i = 0; i < jumlah; ++i) {
                        auto* node = kotak(lebarTombol, 22.f, tema.tombol, 255, true);
                        auto* garis = kotak(lebarTombol, 2.f, tema.aksen, 255);
                        garis->setPosition({0.f, 0.f});
                        node->addChild(garis);
                        auto* ls = teks(b.tombol[i].first, "chatFont.fnt", 0.6f, lebarTombol - 6.f, tema.teks, ccp(0.5f, 0.5f));
                        ls->setPosition({lebarTombol / 2.f, 11.f});
                        node->addChild(ls);
                        auto* item = CCMenuItemExt::createSpriteExtra(node, [fn = b.tombol[i].second](auto) { fn(); });
                        item->m_scaleMultiplier = 1.06f;
                        item->setPosition({4.f + lebarTombol / 2.f + static_cast<float>(i) * (lebarTombol + 4.f), tengahY});
                        menu->addChild(item);
                    }
                    break;
                }
            }
            y -= h;
            ++urut;
        }

        // Posisi scroll: mulai dari atas, atau pulihkan posisi sebelum dibangun ulang
        skrol->scrollToTop();
        if (auto it = simpan.find(idx); it != simpan.end() && tinggiKonten > tinggiLihat) {
            float dari = std::clamp(it->second, 0.f, tinggiKonten - tinggiLihat);
            skrol->m_contentLayer->setPositionY((tinggiLihat - tinggiKonten) + dari);
        }
        m_isi->addChild(skrol, 3);

        // Penunjuk posisi scroll (garis tipis di tepi kanan) hanya bila isi lebih tinggi dari panel
        Skrol info{skrol, nullptr, tinggiKonten, tinggiLihat, areaAtas - HEADER, 0.f, idx};
        if (tinggiKonten > tinggiLihat + 1.f) {
            info.tinggiBar = std::max(14.f, tinggiLihat * tinggiLihat / tinggiKonten);
            info.bar = kotak(2.5f, info.tinggiBar, tema.aksen, 200);
            info.bar->setPosition({x + lebar - 4.f, info.atasY - info.tinggiBar});
            m_isi->addChild(info.bar, 6);
        }
        m_skrol.push_back(info);
    }

    // Anak baru harus ikut prioritas sentuh popup supaya bisa ditekan
    handleTouchPriority(this);
}

void TeamPopup::segarkanCentang() {
    for (auto& cg : m_centang) cg.node->setColor(cg.ambil() ? cg.hidup : cg.mati);
}

void TeamPopup::segarkan(float) {
    if (m_menutup) return;
    if (m_bangunUlang) {
        m_bangunUlang = false;
        this->susun();
        return;
    }
    for (auto& [label, fn] : m_dinamis) label->setString(fn().c_str());
    this->segarkanCentang();
}

void TeamPopup::update(float dt) {
    Popup::update(dt);
    for (auto& s : m_skrol) {
        if (!s.bar) continue;
        float jarak = s.tinggiIsi - s.tinggiLihat;
        if (jarak <= 0.f) continue;
        float jauh = s.layer->m_contentLayer->getPositionY() - (s.tinggiLihat - s.tinggiIsi);   // 0 = paling atas
        float f = std::clamp(jauh / jarak, 0.f, 1.f);
        s.bar->setPositionY(s.atasY - s.tinggiBar - f * (s.tinggiLihat - s.tinggiBar));
    }
}

// Animasi buka: membesar halus (dimatikan di Mode Ringan)
void TeamPopup::show() {
    Popup::show();
    if (!tgm::cfg().modeRingan) {
        m_mainLayer->setScale(m_skala * 0.94f);
        m_mainLayer->runAction(CCEaseSineOut::create(CCScaleTo::create(0.15f, m_skala)));
    }
}

void TeamPopup::onClose(CCObject*) {
    if (m_menutup) return;
    m_menutup = true;
    this->unschedule(schedule_selector(TeamPopup::segarkan));
    this->unscheduleUpdate();
    Popup::onClose(nullptr);
}

// Semua panel menu. Menambah fitur = menambah baris di sini.
std::vector<TeamPopup::Panel> TeamPopup::bangunPanel(TeamPopup* self) {
    auto& c = tgm::cfg();
    std::vector<Panel> hasil;
    auto persen = [](int v) { return fmt::format("{}%", v); };

    // ================= GLOBAL =================
    {
        Panel p{"Global", {}};
        auto& r = p.baris;
        r.push_back(tog("Mode Ringan", c.modeRingan));
        r.push_back(info("Matikan partikel, guncangan, efek mati"));
        r.push_back(stepKecepatan());
        r.push_back(tombol({
            {"0.5x", [&c] { c.speed = 0.5f; tgm::saveConfig(); }},
            {"1x",   [&c] { c.speed = 1.f;  tgm::saveConfig(); }},
            {"2x",   [&c] { c.speed = 2.f;  tgm::saveConfig(); }},
        }));
        r.push_back(tog("Mode Aman Otomatis", c.autoSafeMode));
        r.push_back(tog("Jeda Level Saat Menu Dibuka", c.jedaSaatBuka));
        r.push_back(tog("Konfirmasi Keluar Level", c.konfirmasiKeluar));
        r.push_back(tog("Tombol Restart di Layar", c.tombolRestart));
        r.push_back(info([] { return tgm::cheatsActive() ? std::string("Cheat aktif: progres TIDAK disimpan") : std::string("Tidak ada cheat aktif"); }));
        hasil.push_back(std::move(p));
    }
    // ================= LEVEL =================
    {
        Panel p{"Level", {}};
        auto& r = p.baris;
        r.push_back(tog("Auto Practice Mode", c.autoPractice));
        r.push_back(tog("Checkpoint Otomatis", c.autoCheckpoint));
        r.push_back(stepInt("Jeda Checkpoint", c.checkpointDtk, 1, 30, 1, [](int v) { return fmt::format("{} dtk", v); }));
        r.push_back(tog("Musik Practice = Normal", c.musikPracticeAsli));
        r.push_back(tog("Tampilkan Hitbox", c.hitbox));
        r.push_back(tog("Hitbox Saat Mati", c.hitboxMati));
        r.push_back(tog("Tanpa Guncangan Layar", c.tanpaGuncang));
        r.push_back(tog("Tanpa Portal Cermin", c.tanpaCermin));
        r.push_back(tog("Tanpa Partikel", c.tanpaPartikel));
        hasil.push_back(std::move(p));
    }
    // ================= PLAYER =================
    {
        Panel p{"Player", {}};
        auto& r = p.baris;
        r.push_back(tog("Noclip", c.noclip));
        r.push_back(tog("Noclip Pemain 1", c.noclipP1));
        r.push_back(tog("Noclip Pemain 2", c.noclipP2));
        r.push_back(stepInt("Batas Tabrakan", c.batasTabrakan, 0, 100, 1, [](int v) { return v > 0 ? std::to_string(v) : std::string("Bebas"); }));
        r.push_back(stepInt("Mati Bila Akurasi <", c.akurasiMin, 0, 99, 5, [](int v) { return v > 0 ? fmt::format("{}%", v) : std::string("Mati"); }));
        r.push_back(info([] {
            auto* pl = PlayLayer::get();
            if (!pl) return std::string("Akurasi tampil di dalam level");
            auto const& s = tgm::stats();
            return fmt::format("Akurasi {:.1f}%  |  Tabrakan {}", s.akurasi(pl->m_gameState.m_currentProgress), s.tabrakan);
        }));
        r.push_back(tog("Lompat Bebas (eksperimental)", c.lompatBebas));
        r.push_back(tog("Tanpa Efek Kematian", c.noDeathEffect));
        r.push_back(tog("Respawn Cepat", c.respawnCepat));
        r.push_back(stepInt("Jeda Respawn", c.respawnMs, 0, 1000, 50, [](int v) { return fmt::format("{} ms", v); }));
        r.push_back(tog("Klik Otomatis (tahan untuk spam)", c.turbo));
        r.push_back(stepInt("Kecepatan Klik Otomatis", c.turboCps, 5, 30, 1, [](int v) { return fmt::format("{} /dtk", v); }));
        r.push_back(stepPilih("Mode Klik", c.cbfMode, NAMA_MODE_KLIK));
        r.push_back(info([] { return std::string(tgm::cbf::status()); }));
        r.push_back(tog("Frame Stepper", c.frameStepper));
        r.push_back(tog("Tampilkan Lintasan (perkiraan)", c.lintasan));
        r.push_back(stepInt("Panjang Lintasan", c.lintasanLangkah, 20, 120, 10, [](int v) { return fmt::format("{} langkah", v); }));
        r.push_back(info("Lintasan: busur lompat kubus saja, perkiraan."));
        hasil.push_back(std::move(p));
    }
    // ================= BYPASS =================
    {
        Panel p{"Bypass", {}};
        auto& r = p.baris;
        r.push_back(tog("FPS Bypass (eksperimental)", c.fpsBypass));
        r.push_back(stepInt("Target FPS", c.fpsTarget, 30, 1000, 30, [](int v) { return fmt::format("{} FPS", v); }));
        r.push_back(tog("Buka Semua Ikon dan Warna", c.bukaSemua));
        r.push_back(info("Hanya tampilan lokal, akun tidak berubah."));
        r.push_back(info("Physics Bypass (TPS): belum tersedia."));
        hasil.push_back(std::move(p));
    }
    // ================= BOT =================
    {
        Panel p{"Bot", {}};
        auto& r = p.baris;
        r.push_back(info([] {
            char const* s = "Siaga";
            if (tgm::bot::mode() == tgm::bot::Mode::Rekam) s = "Merekam";
            if (tgm::bot::mode() == tgm::bot::Mode::Putar) s = "Memutar";
            return fmt::format("{} | {} aksi | {:.1f} dtk", s, tgm::bot::jumlahAksi(), tgm::bot::durasiDetik());
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
        r.push_back(tog("Simpan Otomatis Saat Selesai", c.autoSaveMacro));
        r.push_back(tog("Putar Otomatis dari Awal", c.autoPlay));
        r.push_back(tog("Muat Macro Otomatis", c.autoMuat));
        hasil.push_back(std::move(p));
    }
    // ================= LABEL (HUD) =================
    {
        Panel p{"Label", {}};
        auto& r = p.baris;
        r.push_back(tog("Tampilkan Label", c.showHud));
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
        hasil.push_back(std::move(p));
    }
    // ================= GAYA LABEL =================
    {
        Panel p{"Gaya Label", {}};
        auto& r = p.baris;
        r.push_back(stepInt("Kepekatan", c.hudOpacity, 10, 100, 10, persen));
        r.push_back(stepInt("Ukuran", c.hudScale, 50, 150, 10, persen));
        r.push_back(stepPilih("Posisi", c.hudCorner, NAMA_SUDUT));
        r.push_back(stepPilih("Warna", c.hudColor, NAMA_WARNA));
        r.push_back(stepPilih("Huruf", c.hudFont, NAMA_FONT));
        r.push_back(tog("Latar Gelap di Belakang Teks", c.hudPanel));
        r.push_back(stepInt("Geser Horizontal", c.hudOffX, -100, 100, 5));
        r.push_back(stepInt("Geser Vertikal", c.hudOffY, -100, 100, 5));
        hasil.push_back(std::move(p));
    }
    // ================= INPUT =================
    {
        Panel p{"Input Viewer", {}};
        auto& r = p.baris;
        r.push_back(tog("Tampilkan Input Viewer", c.ivTampil));
        r.push_back(stepPilih("Posisi", c.ivPosisi, NAMA_SUDUT));
        r.push_back(stepInt("Ukuran", c.ivUkuran, 50, 200, 10, persen));
        r.push_back(stepInt("Kepekatan", c.ivOpasitas, 20, 100, 10, persen));
        r.push_back(stepPilih("Warna Tombol", c.ivWarna, NAMA_WARNA));
        r.push_back(stepInt("Geser Horizontal", c.ivOffX, -150, 150, 5));
        r.push_back(stepInt("Geser Vertikal", c.ivOffY, -150, 150, 5));
        hasil.push_back(std::move(p));
    }
    // ================= INTERFACE =================
    {
        Panel p{"Interface", {}};
        auto& r = p.baris;
        r.push_back(judul("Menu"));
        r.push_back(stepPilih("Tema (warna aksen)", c.tema, NAMA_TEMA, self));
        r.push_back(tog("Mode Terang", c.menuTerang, self));
        r.push_back(stepInt("Kepekatan Panel", c.menuOpasitas, 40, 100, 10, persen, self));
        r.push_back(stepInt("Ukuran Menu", c.menuSkala, 60, 130, 10, persen));
        r.push_back(info("Ukuran berlaku saat menu dibuka ulang."));
        r.push_back(judul("Tombol Logo"));
        r.push_back(tog("Tampilkan Tombol Logo", c.logoTampil));
        r.push_back(tog("Tampil Saat di Level", c.logoDiLevel));
        r.push_back(tog("Menempel ke Tepi Layar", c.logoMagnet));
        r.push_back(tog("Meredup Otomatis", c.logoRedup));
        r.push_back(stepInt("Ukuran Logo", c.logoUkuran, 50, 200, 10, persen));
        r.push_back(stepInt("Kepekatan Logo", c.logoOpasitas, 20, 100, 10, persen));
        r.push_back(tombol({{"Reset Posisi Logo", [] { FloatingLogo::resetPosisi(); beriTahu("Posisi tombol dikembalikan"); }}}));
        hasil.push_back(std::move(p));
    }
    // ================= SHORTCUTS =================
    {
        Panel p{"Shortcuts", {}};
        auto& r = p.baris;
        r.push_back(info("Aksi cepat untuk level yang sedang dibuka."));
        r.push_back(tombol({{"Restart Level", [] {
            auto* pl = PlayLayer::get();
            if (!pl) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            pl->resetLevelFromStart();
        }}}));
        r.push_back(tombol({{"Toggle Practice Mode", [] {
            auto* pl = PlayLayer::get();
            if (!pl) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            pl->togglePracticeMode(!pl->m_isPracticeMode);
        }}}));
        r.push_back(tombol({{"Pasang Checkpoint", [] {
            auto* pl = PlayLayer::get();
            if (!pl || !pl->m_isPracticeMode) return beriTahu("Hanya di mode practice", NotificationIcon::Warning);
            pl->markCheckpoint();
        }}}));
        r.push_back(tombol({{"Hapus Checkpoint", [] {
            auto* pl = PlayLayer::get();
            if (!pl || !pl->m_isPracticeMode) return beriTahu("Hanya di mode practice", NotificationIcon::Warning);
            if (pl->m_uiLayer) pl->m_uiLayer->onDeleteCheck(nullptr);
        }}}));
        hasil.push_back(std::move(p));
    }
    // ================= STATISTIK =================
    {
        Panel p{"Statistik", {}};
        auto& r = p.baris;
        r.push_back(judul("Keseluruhan"));
        r.push_back(info([] {
            auto t = tgm::riwayat::total();
            return fmt::format("Main {} | Sesi {}", formatDurasi(t.detik), t.sesi);
        }));
        r.push_back(info([] {
            auto t = tgm::riwayat::total();
            return fmt::format("Mati {} | Klik {}", t.mati, t.klik);
        }));
        r.push_back(info([] {
            auto t = tgm::riwayat::total();
            return fmt::format("Percobaan {} | Selesai {}", t.percobaan, t.selesai);
        }));
        r.push_back(judul("Level Ini"));
        r.push_back(info([] {
            int id = idLevelSaatIni();
            if (id < 0) return std::string("Buka dari dalam level");
            auto l = tgm::riwayat::level(id);
            if (!l.ada) return std::string("Belum ada data");
            return fmt::format("Percobaan {} | Mati {} | Terbaik {}%", l.percobaan, l.mati, l.terbaik);
        }));
        r.push_back(info([] {
            int id = idLevelSaatIni();
            if (id < 0) return std::string("");
            auto l = tgm::riwayat::level(id);
            return fmt::format("Klik {} | Selesai {}x", l.klik, l.selesai);
        }));
        r.push_back(info([] {
            int id = idLevelSaatIni();
            if (id < 0) return std::string("");
            auto z = tgm::riwayat::zonaTerbanyak(id);
            if (z.kotak < 0) return std::string("Belum ada data tempat mati");
            return fmt::format("Sering mati di {}-{}% ({}x)", z.kotak * 5, z.kotak * 5 + 5, z.jumlah);
        }));
        r.push_back(info([] {
            auto const& s = tgm::sesi();
            auto menit = static_cast<long long>(std::difftime(std::time(nullptr), s.mulai) / 60.0);
            return fmt::format("Sesi: {} mnt | Mati {} | Klik {}", menit, s.mati, s.klik);
        }));
        hasil.push_back(std::move(p));
    }
    // ================= DATA =================
    {
        Panel p{"Data", {}};
        auto& r = p.baris;
        r.push_back(info([] { return "Status: " + tgm::db::status(); }));
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
        r.push_back(info("Mode Aman mencegah progres tersimpan saat cheat aktif."));
        r.push_back(info("Jangan kirim rekor ke leaderboard memakai cheat."));
        hasil.push_back(std::move(p));
    }
    return hasil;
}

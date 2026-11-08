#include "MegaMenu.hpp"
#include "FloatingLogo.hpp"

#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <set>
#include <string>

#include "../bot/Bot.hpp"
#include "../core/Cheats.hpp"
#include "../core/Config.hpp"
#include "../core/Db.hpp"
#include "../core/Keybinds.hpp"
#include "../core/Riwayat.hpp"
#include "../core/Stats.hpp"
#include "../hacks/Cbf.hpp"
#include "../hacks/Extra.hpp"
#include "../hacks/Stepper.hpp"

using namespace geode::prelude;

namespace {
    // ---- Ukuran (satuan logis, sebelum diskalakan "Ukuran Menu") ----
    constexpr float LEBAR     = 126.f;   // lebar satu jendela
    constexpr float JARAK     = 5.f;     // jarak antar jendela
    constexpr float MARGIN    = 6.f;     // jarak jendela pertama dari tepi kiri
    constexpr float T_HEADER  = 17.f;    // tinggi judul jendela
    constexpr float T_BARIS   = 15.5f;   // tinggi satu baris
    constexpr float T_BAR     = 22.f;    // tinggi bilah judul menu di paling atas
    constexpr char const* ID_MENU = "tgm-mega-menu";

    using Baris = MegaMenu::Baris;
    using Jenis = MegaMenu::Baris::Jenis;

    std::vector<std::string> const NAMA_SUDUT = {"Kiri Atas", "Kanan Atas", "Kiri Bawah", "Kanan Bawah", "Tengah Atas", "Tengah Bawah"};
    std::vector<std::string> const NAMA_WARNA = {"Putih", "Cyan", "Hijau", "Kuning", "Merah", "Oranye", "Ungu", "Pink"};
    std::vector<std::string> const NAMA_FONT  = {"Kecil", "Besar", "Emas"};
    std::vector<std::string> const NAMA_MODE_KLIK = {"Bawaan GD", "Klik di Langkah", "Click Between Frames"};
    std::vector<std::string> const NAMA_TEMA  = {"Ungu", "Biru", "Hijau", "Merah", "Oranye", "Pink", "Cyan", "Abu-abu"};

    // Warna judul jendela sesuai tema pilihan
    ccColor3B warnaAksen() {
        static ccColor3B const T[] = {
            {150, 40, 170}, {45, 110, 225}, {40, 165, 85}, {200, 45, 50},
            {225, 125, 35}, {220, 80, 150}, {35, 175, 195}, {105, 105, 115},
        };
        return T[std::clamp(tgm::cfg().temaMenu, 0, 7)];
    }

    GLubyte opasitasLatar() {
        return static_cast<GLubyte>(std::clamp(tgm::cfg().opasitasMenu, 30, 100) * 255 / 100);
    }

    void beriTahu(std::string const& pesan, NotificationIcon ikon = NotificationIcon::None) {
        Notification::create(pesan, ikon, 1.8f)->show();
    }

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

    // Area yang memotong gambar anak-anaknya (isi jendela yang bisa digulir)
    class MmKlip : public CCNode {
    public:
        CCSize m_area;

        static MmKlip* create(CCSize area) {
            auto* r = new MmKlip();
            if (r->init()) {
                r->m_area = area;
                r->autorelease();
                return r;
            }
            delete r;
            return nullptr;
        }

        void visit() override {
            if (!this->isVisible() || !this->getParent()) {
                CCNode::visit();
                return;
            }
            glEnable(GL_SCISSOR_TEST);
            auto a = this->getParent()->convertToWorldSpace(this->getPosition());
            auto b = this->getParent()->convertToWorldSpace(this->getPosition() + ccp(m_area.width, m_area.height));
            CCDirector::get()->getOpenGLView()->setScissorInPoints(a.x, a.y, b.x - a.x, b.y - a.y);
            CCNode::visit();
            glDisable(GL_SCISSOR_TEST);
        }
    };

    CCLabelBMFont* buatLabel(CCNode* induk, std::string const& teks, float x, float y, float skala,
                             float lebarMaks, bool kiri, ccColor3B warna) {
        auto* l = CCLabelBMFont::create(teks.c_str(), "chatFont.fnt");
        l->setScale(skala);
        if (lebarMaks > 0.f) l->limitLabelWidth(lebarMaks, skala, 0.12f);
        l->setAnchorPoint({kiri ? 0.f : 0.5f, 0.5f});
        l->setPosition({x, y});
        l->setColor(warna);
        induk->addChild(l, 2);
        return l;
    }

    // Susun baris (beserta anak yang sedang dibuka) menjadi daftar datar berurutan dari atas
    struct DatarLokal { Baris* baris; int tingkat; };
    void ratakanKe(std::vector<Baris>& v, int tingkat, std::vector<DatarLokal>& hasil) {
        for (auto& b : v) {
            hasil.push_back({&b, tingkat});
            if (b.terbuka && !b.anak.empty()) ratakanKe(b.anak, tingkat + 1, hasil);
        }
    }

    // Beri tiap baris id "Jendela/Teks" (dipakai keybind dan untuk mengingat baris yang terbuka)
    void tandaiKunci(std::string const& jendela, std::vector<Baris>& v) {
        for (auto& b : v) {
            b.kunciRow = jendela + "/" + b.teks;
            if (!b.anak.empty()) tandaiKunci(jendela, b.anak);
        }
    }

    void kumpulTerbuka(std::vector<Baris> const& v, std::set<std::string>& hasil) {
        for (auto const& b : v) {
            if (b.terbuka) hasil.insert(b.kunciRow);
            if (!b.anak.empty()) kumpulTerbuka(b.anak, hasil);
        }
    }

    void pulihTerbuka(std::vector<Baris>& v, std::set<std::string> const& dulu) {
        for (auto& b : v) {
            if (!b.anak.empty() && dulu.count(b.kunciRow)) b.terbuka = true;
            if (!b.anak.empty()) pulihTerbuka(b.anak, dulu);
        }
    }

    // ---- Pembuat baris ----
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

    // Toggle dengan sub-pengaturan (panah di kanan membuka / menutup anak)
    Baris togAnak(std::string teks, bool& ref, std::vector<Baris> anak) {
        auto b = tog(std::move(teks), ref);
        b.anak = std::move(anak);
        return b;
    }

    // Baris pengelompok tanpa toggle: hanya membuka / menutup anak
    Baris grup(std::string teks, std::vector<Baris> anak) {
        Baris b; b.jenis = Jenis::Grup; b.teks = std::move(teks); b.anak = std::move(anak); return b;
    }

    Baris stepInt(std::string teks, int& ref, int mn, int mx, int langkah, std::function<std::string(int)> fmtNilai = nullptr,
                  std::function<void()> sesudah = nullptr) {
        Baris b; b.jenis = Jenis::Stepper; b.teks = std::move(teks);
        b.nilai = [&ref, fmtNilai] { return fmtNilai ? fmtNilai(ref) : std::to_string(ref); };
        b.ubah = [&ref, mn, mx, langkah, sesudah](int arah) {
            ref = std::clamp(ref + arah * langkah, mn, mx);
            tgm::saveConfig();
            if (sesudah) sesudah();
        };
        return b;
    }

    Baris stepPilih(std::string teks, int& ref, std::vector<std::string> const& nama) {
        Baris b; b.jenis = Jenis::Stepper; b.teks = std::move(teks);
        int n = static_cast<int>(nama.size());
        b.nilai = [&ref, &nama, n] { return nama[static_cast<std::size_t>(std::clamp(ref, 0, n - 1))]; };
        b.ubah = [&ref, n](int arah) { ref = (ref + arah + n) % n; tgm::saveConfig(); };
        return b;
    }

    Baris stepKecepatan() {
        auto& c = tgm::cfg();
        Baris b; b.jenis = Jenis::Stepper; b.teks = "Speedhack";
        b.nilai = [&c] { return fmt::format("{:.1f}x", c.speed); };
        b.ubah = [&c](int arah) {
            c.speed = std::clamp(std::round((c.speed + 0.1f * static_cast<float>(arah)) * 10.f) / 10.f, 0.1f, 3.f);
            tgm::saveConfig();
        };
        return b;
    }

    // Baris keybind: ketuk = pasang tombol baru, ketuk "x" di kanan = hapus
    Baris kunci(std::string teks, std::string id) {
        Baris b; b.jenis = Jenis::Kunci; b.teks = teks;
        b.nilai = [id] {
            if (tgm::kb::sedangMenangkap(id)) return std::string("...");
            return tgm::kb::namaKunci(tgm::kb::tombolDari(id));
        };
        b.aksi = [id, teks] {
            tgm::kb::mulaiTangkap(id, teks, [] { MegaMenu::susunUlangJikaTerbuka(); });
        };
        b.ubah = [id](int) {
            tgm::kb::hapus(id);
            MegaMenu::susunUlangJikaTerbuka();
        };
        return b;
    }

    Baris tombol(std::string teks, std::function<void()> fn) {
        Baris b; b.jenis = Jenis::Tombol; b.teks = std::move(teks); b.aksi = std::move(fn); return b;
    }

    // Aksi pintasan di dalam level: tutup menu dulu agar level terlihat
    PlayLayer* levelUntukAksi() {
        auto* pl = PlayLayer::get();
        if (!pl) beriTahu("Buka dari dalam level", NotificationIcon::Warning);
        return pl;
    }
}

// ============================================================================
// Siklus hidup
// ============================================================================

MegaMenu* MegaMenu::create() {
    auto ret = new MegaMenu();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool MegaMenu::terbuka() {
    auto* scene = CCDirector::get()->getRunningScene();
    return scene && scene->getChildByID(ID_MENU) != nullptr;
}

void MegaMenu::bukaAtauTutup() {
    auto* scene = CCDirector::get()->getRunningScene();
    if (!scene) return;

    // Sudah terbuka: tutup
    if (auto* ada = typeinfo_cast<MegaMenu*>(scene->getChildByID(ID_MENU))) {
        ada->tutup();
        return;
    }

    // Di dalam level yang sedang berjalan: jeda dulu agar pemain tidak mati saat menu dibuka
    if (auto* pl = PlayLayer::get(); pl && tgm::cfg().jedaSaatBuka && !pl->m_isPaused) {
        pl->pauseGame(false);
    }

    if (auto* menu = MegaMenu::create()) scene->addChild(menu, 2000);
}

float MegaMenu::skala() const {
    return std::clamp(static_cast<float>(tgm::cfg().skalaUi) / 100.f, 0.5f, 1.5f);
}

bool MegaMenu::init() {
    if (!CCLayer::init()) return false;
    this->setID(ID_MENU);
    this->setTouchMode(kCCTouchesOneByOne);
    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);

    auto win = CCDirector::get()->getWinSize();
    this->setContentSize(win);
    this->setPosition({0.f, 0.f});

    m_dim = CCLayerColor::create(ccc4(0, 0, 0, 130), win.width, win.height);
    this->addChild(m_dim, -1);

    m_dunia = CCNode::create();
    this->addChild(m_dunia, 1);

    this->susunJendela();
    this->bangunUlang();

    if (!tgm::cfg().modeRingan) {
        m_dim->setOpacity(0);
        m_dim->runAction(CCFadeTo::create(0.12f, 130));
    }
    this->schedule(schedule_selector(MegaMenu::segarkan), 0.25f);
    return true;
}

void MegaMenu::registerWithTouchDispatcher() {
    // Di bawah tombol logo mengambang (-9999) tetapi di atas popup dan level
    CCTouchDispatcher::get()->addTargetedDelegate(this, -600, true);
}

void MegaMenu::keyBackClicked() {
    this->tutup();
}

void MegaMenu::tutup() {
    if (m_menutup) return;
    m_menutup = true;
    this->setTouchEnabled(false);
    this->unschedule(schedule_selector(MegaMenu::segarkan));

    if (tgm::cfg().modeRingan) {
        this->removeFromParentAndCleanup(true);
        return;
    }
    m_dunia->setVisible(false);
    m_dim->runAction(CCFadeTo::create(0.08f, 0));
    this->runAction(CCSequence::create(
        CCDelayTime::create(0.08f),
        CCCallFunc::create(this, callfunc_selector(MegaMenu::hapus)),
        nullptr));
}

void MegaMenu::hapus() {
    this->removeFromParentAndCleanup(true);
}

void MegaMenu::segarkan(float) {
    if (m_menutup) return;
    for (auto& d : m_dinamis) {
        d.label->setString(d.fn().c_str());
        if (d.lebarMaks > 0.f) d.label->limitLabelWidth(d.lebarMaks, d.skala, 0.12f);
    }
}

// ============================================================================
// Gambar
// ============================================================================

float MegaMenu::gulirMaks(std::size_t i) const {
    auto const& j = m_jendela[i];
    return std::max(0.f, j.tinggiIsi - j.tinggiBody);
}

void MegaMenu::aturPosisiX() {
    for (std::size_t i = 0; i < m_akarJendela.size(); ++i) {
        if (m_akarJendela[i]) {
            m_akarJendela[i]->setPositionX(MARGIN + m_gulirX + static_cast<float>(i) * (LEBAR + JARAK));
        }
    }
}

void MegaMenu::perbaruiGagang(std::size_t i) {
    if (i >= m_gagang.size() || !m_gagang[i]) return;
    auto const& j = m_jendela[i];
    float maks = this->gulirMaks(i);
    if (maks <= 0.f || j.tinggiBody <= 0.f) {
        m_gagang[i]->setVisible(false);
        return;
    }
    float tinggi = std::max(10.f, j.tinggiBody * j.tinggiBody / j.tinggiIsi);
    float rasio = std::clamp(j.gulir / maks, 0.f, 1.f);
    m_gagang[i]->setVisible(true);
    m_gagang[i]->setContentSize({2.f, tinggi});
    m_gagang[i]->setPositionY((j.tinggiBody - tinggi) * (1.f - rasio));
}

void MegaMenu::gambarBaris(CCNode* isi, Datar const& d, float y, int indeks) {
    auto aksen = warnaAksen();
    auto& b = *d.baris;
    float indent = 8.f * static_cast<float>(d.tingkat);
    float tinggi = T_BARIS - 1.f;
    float tengahY = y + tinggi / 2.f;
    bool punyaAnak = !b.anak.empty();
    float lebarPanah = punyaAnak ? 14.f : 0.f;

    auto* latar = CCLayerColor::create(indeks % 2 == 0 ? ccc4(26, 26, 30, 255) : ccc4(21, 21, 25, 255), LEBAR - 2.f, tinggi);
    latar->setPosition({1.f, y});
    isi->addChild(latar, 0);

    if (punyaAnak) {
        buatLabel(isi, b.terbuka ? "v" : ">", LEBAR - 9.f, tengahY, 0.5f, 0.f, false, {200, 200, 210});
    }

    switch (b.jenis) {
        case Jenis::Judul:
            buatLabel(isi, b.teks, 5.f + indent, tengahY, 0.44f, LEBAR - 10.f, true, aksen);
            break;

        case Jenis::Info: {
            auto* l = buatLabel(isi, b.nilai(), 5.f + indent, tengahY, 0.4f, LEBAR - 10.f - indent, true, {150, 150, 162});
            m_dinamis.push_back({l, b.nilai, LEBAR - 10.f - indent, 0.4f});
            break;
        }

        case Jenis::Toggle: {
            bool aktif = b.ambil();
            auto* garis = CCLayerColor::create(ccc4(75, 75, 85, 255), 9.f, 9.f);
            garis->setPosition({4.f + indent, tengahY - 4.5f});
            isi->addChild(garis, 1);
            auto* dalam = CCLayerColor::create(aktif ? ccc4(aksen.r, aksen.g, aksen.b, 255) : ccc4(18, 18, 21, 255), 7.f, 7.f);
            dalam->setPosition({1.f, 1.f});
            garis->addChild(dalam);
            buatLabel(isi, b.teks, 17.f + indent, tengahY, 0.46f, LEBAR - 21.f - indent - lebarPanah, true,
                      aktif ? ccColor3B{240, 240, 245} : ccColor3B{190, 190, 198});
            break;
        }

        case Jenis::Grup:
            buatLabel(isi, b.teks, 5.f + indent, tengahY, 0.46f, LEBAR - 10.f - indent - lebarPanah, true, {240, 240, 245});
            break;

        case Jenis::Stepper: {
            buatLabel(isi, b.teks, 5.f + indent, tengahY, 0.46f, LEBAR - 70.f - indent, true, {225, 225, 232});
            auto* kotak = CCLayerColor::create(ccc4(40, 40, 46, 255), 58.f, tinggi - 3.f);
            kotak->setPosition({LEBAR - 62.f, y + 1.5f});
            isi->addChild(kotak, 1);
            buatLabel(isi, "-", LEBAR - 62.f + 6.f, tengahY, 0.55f, 0.f, false, {210, 210, 220});
            buatLabel(isi, "+", LEBAR - 62.f + 52.f, tengahY, 0.55f, 0.f, false, {210, 210, 220});
            auto* nilai = buatLabel(isi, b.nilai(), LEBAR - 62.f + 29.f, tengahY, 0.42f, 36.f, false, {255, 255, 255});
            m_dinamis.push_back({nilai, b.nilai, 36.f, 0.42f});
            break;
        }

        case Jenis::Kunci: {
            buatLabel(isi, b.teks, 5.f + indent, tengahY, 0.46f, LEBAR - 70.f - indent, true, {225, 225, 232});
            auto* kotak = CCLayerColor::create(ccc4(40, 40, 46, 255), 58.f, tinggi - 3.f);
            kotak->setPosition({LEBAR - 62.f, y + 1.5f});
            isi->addChild(kotak, 1);
            buatLabel(isi, "x", LEBAR - 62.f + 52.f, tengahY, 0.5f, 0.f, false, {200, 90, 90});
            auto* nilai = buatLabel(isi, b.nilai(), LEBAR - 62.f + 24.f, tengahY, 0.42f, 40.f, false, {255, 255, 255});
            m_dinamis.push_back({nilai, b.nilai, 40.f, 0.42f});
            break;
        }

        case Jenis::Tombol: {
            float lebar = LEBAR - 8.f - indent;
            auto* tbl = CCLayerColor::create(ccc4(static_cast<GLubyte>(aksen.r * 6 / 10), static_cast<GLubyte>(aksen.g * 6 / 10),
                                                  static_cast<GLubyte>(aksen.b * 6 / 10), 255), lebar, tinggi - 2.f);
            tbl->setPosition({4.f + indent, y + 1.f});
            isi->addChild(tbl, 1);
            buatLabel(isi, b.teks, 4.f + indent + lebar / 2.f, tengahY, 0.46f, lebar - 6.f, false, {245, 245, 250});
            break;
        }
    }
}

void MegaMenu::bangunUlang() {
    m_dunia->removeAllChildren();
    m_dinamis.clear();
    m_akarJendela.assign(m_jendela.size(), nullptr);
    m_isiJendela.assign(m_jendela.size(), nullptr);
    m_gagang.assign(m_jendela.size(), nullptr);

    auto win = CCDirector::get()->getWinSize();
    float s = this->skala();
    m_dunia->setScale(s);
    m_lw = win.width / s;
    m_lh = win.height / s;
    auto aksen = warnaAksen();
    auto& c = tgm::cfg();

    // ---- Bilah judul menu ----
    auto* bar = CCLayerColor::create(ccc4(0, 0, 0, 175), m_lw, T_BAR);
    bar->setPosition({0.f, m_lh - T_BAR});
    m_dunia->addChild(bar, 5);

    float tengahBar = m_lh - T_BAR / 2.f;
    if (auto* logo = CCSprite::create("logo.png"_spr)) {
        logo->setScale(16.f / std::max(1.f, logo->getContentSize().width));
        logo->setPosition({14.f, tengahBar});
        m_dunia->addChild(logo, 6);
    }
    buatLabel(m_dunia, fmt::format("TeamGdMod  v{}", Mod::get()->getVersion().toVString(false)), 26.f, tengahBar, 0.55f, 0.f, true, {255, 255, 255})->setZOrder(6);
    auto* garisAksen = CCLayerColor::create(ccc4(aksen.r, aksen.g, aksen.b, 255), m_lw, 1.5f);
    garisAksen->setPosition({0.f, m_lh - T_BAR - 1.5f});
    m_dunia->addChild(garisAksen, 6);

    // Tombol tutup
    m_rectTutup = CCRect(m_lw - 32.f, m_lh - T_BAR + 3.f, 26.f, T_BAR - 6.f);
    auto* tutup = CCLayerColor::create(ccc4(175, 40, 46, 255), m_rectTutup.size.width, m_rectTutup.size.height);
    tutup->setPosition(m_rectTutup.origin);
    m_dunia->addChild(tutup, 6);
    buatLabel(m_dunia, "X", m_rectTutup.origin.x + m_rectTutup.size.width / 2.f, tengahBar, 0.6f, 0.f, false, {255, 255, 255})->setZOrder(7);

    // ---- Jendela ----
    m_atas = m_lh - T_BAR - 5.f;
    float tinggiMaks = m_atas - 6.f;
    float totalLebar = static_cast<float>(m_jendela.size()) * (LEBAR + JARAK) - JARAK + MARGIN * 2.f;
    m_gulirX = std::clamp(m_gulirX, std::min(0.f, m_lw - totalLebar), 0.f);

    for (std::size_t i = 0; i < m_jendela.size(); ++i) {
        auto& j = m_jendela[i];

        std::vector<DatarLokal> datar;
        ratakanKe(j.baris, 0, datar);

        j.tinggiIsi = static_cast<float>(datar.size()) * T_BARIS + 2.f;
        j.tinggiBody = j.tutup ? 0.f : std::min(j.tinggiIsi, tinggiMaks - T_HEADER);
        j.gulir = std::clamp(j.gulir, 0.f, this->gulirMaks(i));
        float tinggiTotal = T_HEADER + j.tinggiBody;

        auto* akar = CCNode::create();
        akar->setPosition({MARGIN + m_gulirX + static_cast<float>(i) * (LEBAR + JARAK), m_atas - tinggiTotal});
        m_dunia->addChild(akar, 2);
        m_akarJendela[i] = akar;

        // Latar isi
        if (!j.tutup) {
            auto* latar = CCLayerColor::create(ccc4(14, 14, 17, opasitasLatar()), LEBAR, j.tinggiBody);
            akar->addChild(latar, 0);
        }

        // Judul jendela: segitiga lipat di kiri, nama di tengah
        auto* kepala = CCLayerColor::create(ccc4(aksen.r, aksen.g, aksen.b, 255), LEBAR, T_HEADER);
        kepala->setPosition({0.f, j.tinggiBody});
        akar->addChild(kepala, 3);
        buatLabel(kepala, j.tutup ? ">" : "v", 8.f, T_HEADER / 2.f, 0.5f, 0.f, false, {255, 255, 255});
        buatLabel(kepala, j.judul, LEBAR / 2.f, T_HEADER / 2.f, 0.52f, LEBAR - 36.f, false, {255, 255, 255});

        if (j.tutup) continue;

        auto* klip = MmKlip::create({LEBAR, j.tinggiBody});
        akar->addChild(klip, 1);
        auto* isi = CCNode::create();
        isi->setPositionY(j.gulir);
        klip->addChild(isi);
        m_isiJendela[i] = isi;

        float y = j.tinggiBody - 1.f;
        int indeks = 0;
        for (auto const& d : datar) {
            y -= T_BARIS;
            this->gambarBaris(isi, Datar{d.baris, d.tingkat}, y, indeks++);
        }

        // Penanda posisi gulir (hanya bila isi lebih panjang dari jendela)
        auto* gagang = CCLayerColor::create(ccc4(aksen.r, aksen.g, aksen.b, 230), 2.f, 10.f);
        gagang->setPosition({LEBAR - 3.f, 0.f});
        akar->addChild(gagang, 4);
        m_gagang[i] = gagang;
        this->perbaruiGagang(i);
    }
}

// ============================================================================
// Sentuhan
// ============================================================================

CCPoint MegaMenu::keLogis(CCTouch* sentuh) const {
    auto p = sentuh->getLocation();
    float s = this->skala();
    return {p.x / s, p.y / s};
}

int MegaMenu::cariJendelaBody(CCPoint p) const {
    for (std::size_t i = 0; i < m_jendela.size(); ++i) {
        auto const& j = m_jendela[i];
        if (j.tutup) continue;
        float x = MARGIN + m_gulirX + static_cast<float>(i) * (LEBAR + JARAK);
        float bawah = m_atas - (T_HEADER + j.tinggiBody);
        if (p.x >= x && p.x <= x + LEBAR && p.y >= bawah && p.y <= bawah + j.tinggiBody) return static_cast<int>(i);
    }
    return -1;
}

bool MegaMenu::ccTouchBegan(CCTouch* sentuh, CCEvent*) {
    if (m_menutup) return true;
    m_mulai = m_akhir = this->keLogis(sentuh);
    m_geser = false;
    m_sumbu = 0;
    m_tahanTerpicu = false;
    m_jendelaSentuh = this->cariJendelaBody(m_mulai);

    // Sedang menunggu tombol keyboard: sentuhan layar membatalkan
    if (tgm::kb::sedangMenangkap()) {
        tgm::kb::batalTangkap();
        m_tahanTerpicu = true;   // abaikan ketukan ini
        return true;
    }

    // Tekan-tahan pada toggle / tombol = pasang keybind (padanan klik kanan di PC)
    this->unschedule(schedule_selector(MegaMenu::tahanSentuh));
    this->scheduleOnce(schedule_selector(MegaMenu::tahanSentuh), 0.6f);
    return true;   // semua sentuhan ditelan agar tidak tembus ke level
}

void MegaMenu::ccTouchMoved(CCTouch* sentuh, CCEvent*) {
    if (m_menutup) return;
    auto p = this->keLogis(sentuh);

    if (!m_geser) {
        float dx = std::abs(p.x - m_mulai.x), dy = std::abs(p.y - m_mulai.y);
        if (std::max(dx, dy) < 7.f) return;
        m_geser = true;
        this->unschedule(schedule_selector(MegaMenu::tahanSentuh));
        bool bisaVertikal = m_jendelaSentuh >= 0 && this->gulirMaks(static_cast<std::size_t>(m_jendelaSentuh)) > 0.f;
        m_sumbu = (dy > dx && bisaVertikal) ? 2 : 1;
        m_akhir = p;   // mulai menggulir dari titik ini agar tidak melompat
        return;
    }

    float dx = p.x - m_akhir.x, dy = p.y - m_akhir.y;
    m_akhir = p;

    if (m_sumbu == 2) {
        auto i = static_cast<std::size_t>(m_jendelaSentuh);
        auto& j = m_jendela[i];
        j.gulir = std::clamp(j.gulir + dy, 0.f, this->gulirMaks(i));
        if (m_isiJendela[i]) m_isiJendela[i]->setPositionY(j.gulir);
        this->perbaruiGagang(i);
    } else {
        float totalLebar = static_cast<float>(m_jendela.size()) * (LEBAR + JARAK) - JARAK + MARGIN * 2.f;
        m_gulirX = std::clamp(m_gulirX + dx, std::min(0.f, m_lw - totalLebar), 0.f);
        this->aturPosisiX();
    }
}

void MegaMenu::ccTouchEnded(CCTouch*, CCEvent*) {
    this->unschedule(schedule_selector(MegaMenu::tahanSentuh));
    if (m_menutup) return;
    if (!m_geser && !m_tahanTerpicu) this->ketuk(m_mulai);
    m_geser = false;
    m_sumbu = 0;
    m_tahanTerpicu = false;
}

void MegaMenu::ccTouchCancelled(CCTouch*, CCEvent*) {
    this->unschedule(schedule_selector(MegaMenu::tahanSentuh));
    m_geser = false;
    m_sumbu = 0;
    m_tahanTerpicu = false;
}

// Baris di bawah titik sentuh (nullptr bila bukan baris); xLokal = posisi x di dalam jendela
MegaMenu::Baris* MegaMenu::cariBaris(CCPoint p, float& xLokal) {
    for (std::size_t i = 0; i < m_jendela.size(); ++i) {
        auto& j = m_jendela[i];
        if (j.tutup) continue;
        float x = MARGIN + m_gulirX + static_cast<float>(i) * (LEBAR + JARAK);
        if (p.x < x || p.x > x + LEBAR) continue;
        float bawah = m_atas - (T_HEADER + j.tinggiBody);
        float batasKepala = bawah + j.tinggiBody;
        if (p.y < bawah || p.y > batasKepala) continue;
        float jarakDariAtas = (batasKepala - 1.f) - p.y + j.gulir;
        if (jarakDariAtas < 0.f) continue;
        auto indeks = static_cast<std::size_t>(std::floor(jarakDariAtas / T_BARIS));
        std::vector<DatarLokal> datar;
        ratakanKe(j.baris, 0, datar);
        if (indeks >= datar.size()) continue;
        xLokal = p.x - x;
        return datar[indeks].baris;
    }
    return nullptr;
}

void MegaMenu::tahanSentuh(float) {
    if (m_menutup || m_geser || tgm::kb::sedangMenangkap()) return;
    float x = 0.f;
    auto* b = this->cariBaris(m_mulai, x);
    if (!b || b->kunciRow.empty()) return;
    if (b->jenis != Jenis::Toggle && b->jenis != Jenis::Tombol) return;
    if (!b->anak.empty() && x > LEBAR - 20.f) return;   // area panah buka / tutup

    m_tahanTerpicu = true;
    tgm::kb::mulaiTangkap(b->kunciRow, b->teks, [] { MegaMenu::susunUlangJikaTerbuka(); });
}

void MegaMenu::ketuk(CCPoint p) {
    Ref<MegaMenu> jaga = this;   // aksi tombol boleh menutup menu

    if (m_rectTutup.containsPoint(p)) {
        this->tutup();
        return;
    }

    for (std::size_t i = 0; i < m_jendela.size(); ++i) {
        auto& j = m_jendela[i];
        float x = MARGIN + m_gulirX + static_cast<float>(i) * (LEBAR + JARAK);
        if (p.x < x || p.x > x + LEBAR) continue;

        float bawah = m_atas - (T_HEADER + j.tinggiBody);
        float batasKepala = bawah + j.tinggiBody;

        // Judul: lipat / buka jendela
        if (p.y > batasKepala && p.y <= m_atas) {
            j.tutup = !j.tutup;
            auto& c = tgm::cfg();
            if (j.tutup) c.mmTutup |= (1 << static_cast<int>(i));
            else c.mmTutup &= ~(1 << static_cast<int>(i));
            tgm::saveConfig();
            this->bangunUlang();
            return;
        }

        // Isi: cari baris yang diketuk
        if (!j.tutup && p.y >= bawah && p.y <= batasKepala) {
            float jarakDariAtas = (batasKepala - 1.f) - p.y + j.gulir;
            if (jarakDariAtas < 0.f) return;
            auto indeks = static_cast<std::size_t>(std::floor(jarakDariAtas / T_BARIS));
            std::vector<DatarLokal> datar;
            ratakanKe(j.baris, 0, datar);
            if (indeks >= datar.size()) return;
            this->aksiBaris(*datar[indeks].baris, p.x - x);
            return;
        }
    }
}

void MegaMenu::aksiBaris(Baris& b, float x) {
    bool punyaAnak = !b.anak.empty();
    switch (b.jenis) {
        case Jenis::Toggle:
            if (punyaAnak && x > LEBAR - 20.f) b.terbuka = !b.terbuka;
            else if (b.setel && b.ambil) b.setel(!b.ambil());
            this->bangunUlang();
            break;

        case Jenis::Grup:
            b.terbuka = !b.terbuka;
            this->bangunUlang();
            break;

        case Jenis::Stepper:
            if (x >= LEBAR - 62.f && b.ubah) {
                b.ubah(x < LEBAR - 33.f ? -1 : 1);
                this->bangunUlang();   // tema / ukuran menu bisa berubah
            }
            break;

        case Jenis::Tombol:
            if (b.aksi) b.aksi();
            break;

        case Jenis::Kunci:
            if (x > LEBAR - 18.f) { if (b.ubah) b.ubah(0); }
            else if (b.aksi) b.aksi();
            break;

        default:
            break;
    }
}

// ============================================================================
// Isi jendela
// ============================================================================

std::vector<MegaMenu::Jendela> MegaMenu::bangunJendela() {
    auto& c = tgm::cfg();
    std::vector<Jendela> hasil;

    auto tambah = [&hasil](std::string namaJendela, std::vector<Baris> baris) {
        Jendela j;
        j.judul = std::move(namaJendela);
        j.baris = std::move(baris);
        hasil.push_back(std::move(j));
    };

    // ================= Global =================
    {
        std::vector<Baris> r;
        r.push_back(tog("Mode Ringan", c.modeRingan));
        r.push_back(info("LDM, tanpa efek mati & guncang"));

        auto fpsTog = tog("FPS Bypass", c.fpsBypass);
        auto dasar = fpsTog.setel;
        fpsTog.setel = [dasar](bool v) { dasar(v); tgm::terapkanFps(); };
        r.push_back(std::move(fpsTog));
        r.push_back(stepInt("FPS Target", c.fpsTarget, 30, 360, 10,
                            [](int v) { return fmt::format("{} fps", v); }, [] { tgm::terapkanFps(); }));

        r.push_back(stepKecepatan());
        r.push_back(tombol("Speed 1x", [&c] { c.speed = 1.f; tgm::saveConfig(); }));
        r.push_back(tombol("Speed +0.1", [&c] {
            c.speed = std::clamp(std::round((c.speed + 0.1f) * 10.f) / 10.f, 0.1f, 3.f);
            tgm::saveConfig();
            beriTahu(fmt::format("Speed {:.1f}x", c.speed));
        }));
        r.push_back(tombol("Speed -0.1", [&c] {
            c.speed = std::clamp(std::round((c.speed - 0.1f) * 10.f) / 10.f, 0.1f, 3.f);
            tgm::saveConfig();
            beriTahu(fmt::format("Speed {:.1f}x", c.speed));
        }));
        r.push_back(tog("Auto Safe Mode", c.autoSafeMode));
        r.push_back(tog("Safe Mode", c.safeMode));
        r.push_back(tog("Jeda Saat Menu Dibuka", c.jedaSaatBuka));
        r.push_back(tog("Konfirmasi Keluar Level", c.konfirmasiKeluar));
        r.push_back(tog("Tombol Restart di Layar", c.tombolRestart));
        r.push_back(info([] {
            return tgm::cheatsActive() ? std::string("Cheat aktif: progres tidak disimpan")
                                       : std::string("Tidak ada cheat aktif");
        }));
        tambah("Global", std::move(r));
    }

    // ================= Level =================
    {
        std::vector<Baris> r;

        std::vector<Baris> noclip;
        noclip.push_back(tog("Pemain 1", c.noclipP1));
        noclip.push_back(tog("Pemain 2", c.noclipP2));
        noclip.push_back(stepInt("Batas Tabrakan", c.batasTabrakan, 0, 100, 1,
                                 [](int v) { return v > 0 ? std::to_string(v) : std::string("Bebas"); }));
        noclip.push_back(stepInt("Mati Jika Akurasi <", c.akurasiMin, 0, 99, 5,
                                 [](int v) { return v > 0 ? fmt::format("{}%", v) : std::string("Mati"); }));
        noclip.push_back(info([] {
            auto* pl = PlayLayer::get();
            if (!pl) return std::string("Akurasi tampil di dalam level");
            auto const& s = tgm::stats();
            return fmt::format("Akurasi {:.1f}%  Tabrak {}", s.akurasi(pl->m_gameState.m_currentProgress), s.tabrakan);
        }));
        r.push_back(togAnak("Noclip", c.noclip, std::move(noclip)));

        r.push_back(tog("Auto Practice Mode", c.autoPractice));
        r.push_back(tog("Auto LDM", c.autoLdm));
        r.push_back(tog("Bekukan Percobaan", c.bekukanPercobaan));

        std::vector<Baris> cp;
        cp.push_back(stepInt("Jeda Checkpoint", c.checkpointDtk, 1, 30, 1, [](int v) { return fmt::format("{} dtk", v); }));
        r.push_back(togAnak("Checkpoint Otomatis", c.autoCheckpoint, std::move(cp)));

        r.push_back(tog("Tanpa Guncangan Layar", c.tanpaGuncang));
        r.push_back(tog("Tanpa Portal Cermin", c.tanpaCermin));

        std::vector<Baris> hb;
        hb.push_back(tog("Hitbox Saat Mati", c.hitboxMati));
        r.push_back(togAnak("Tampilkan Hitbox", c.hitbox, std::move(hb)));

        r.push_back(tog("Frame Stepper", c.frameStepper));
        r.push_back(tombol("Stepper: Jeda/Lanjut", [&c] {
            if (!PlayLayer::get()) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            if (!c.frameStepper) return beriTahu("Nyalakan Frame Stepper dulu", NotificationIcon::Warning);
            tgm::stepper::ubahBeku();
            beriTahu(tgm::stepper::beku() ? "Level dibekukan" : "Level dilanjutkan");
        }));
        r.push_back(tombol("Stepper: Langkah", [&c] {
            if (!PlayLayer::get()) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            if (!c.frameStepper) return beriTahu("Nyalakan Frame Stepper dulu", NotificationIcon::Warning);
            tgm::stepper::langkah();
        }));
        r.push_back(tog("Instant Mirror", c.instantMirror));

        std::vector<Baris> ak;
        ak.push_back(stepInt("Mati di Persen", c.autoKillPersen, 1, 100, 1, [](int v) { return fmt::format("{}%", v); }));
        r.push_back(togAnak("Auto Kill", c.autoKill, std::move(ak)));

        r.push_back(tog("Musik Practice Sinkron", c.musikPracticeAsli));
        tambah("Level", std::move(r));
    }

    // ================= Player =================
    {
        std::vector<Baris> r;
        r.push_back(tog("Jump Hack", c.lompatBebas));
        r.push_back(tog("Tanpa Efek Mati", c.noDeathEffect));
        r.push_back(tog("Mute Audio Saat Mati", c.muteMati));

        std::vector<Baris> rs;
        rs.push_back(stepInt("Jeda Respawn", c.respawnMs, 0, 1000, 50, [](int v) { return fmt::format("{} ms", v); }));
        r.push_back(togAnak("Respawn Cepat", c.respawnCepat, std::move(rs)));

        std::vector<Baris> ac;
        ac.push_back(stepInt("Klik per Detik", c.turboCps, 5, 30, 1, [](int v) { return fmt::format("{}/dtk", v); }));
        r.push_back(togAnak("Auto Clicker", c.turbo, std::move(ac)));

        std::vector<Baris> jj;
        jj.push_back(stepInt("Panjang", c.jejakPanjang, 50, 600, 50));
        jj.push_back(stepPilih("Warna", c.jejakWarna, NAMA_WARNA));
        r.push_back(togAnak("Jejak Lintasan", c.jejak, std::move(jj)));

        r.push_back(stepPilih("Mode Klik", c.cbfMode, NAMA_MODE_KLIK));
        r.push_back(info([] { return std::string(tgm::cbf::status()); }));
        tambah("Player", std::move(r));
    }

    // ================= Bypass =================
    {
        std::vector<Baris> r;
        r.push_back(tog("Buka Semua Ikon & Warna", c.bukaSemua));
        r.push_back(info("Hanya tampilan lokal"));
        r.push_back(tog("Practice Music Sync", c.musikPracticeAsli));
        r.push_back(tog("Tanpa Portal Cermin", c.tanpaCermin));
        r.push_back(tog("Tanpa Guncangan", c.tanpaGuncang));
        tambah("Bypass", std::move(r));
    }

    // ================= Bot =================
    {
        std::vector<Baris> r;
        r.push_back(info([] {
            char const* s = "Siaga";
            if (tgm::bot::mode() == tgm::bot::Mode::Rekam) s = "Merekam";
            if (tgm::bot::mode() == tgm::bot::Mode::Putar) s = "Memutar";
            return fmt::format("{} | {} aksi | {:.1f} dtk", s, tgm::bot::jumlahAksi(), tgm::bot::durasiDetik());
        }));
        r.push_back(tombol("Rekam", [] {
            if (idLevelSaatIni() < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            tgm::bot::mulaiRekam();
            beriTahu("Merekam: restart level lalu main", NotificationIcon::Success);
        }));
        r.push_back(tombol("Putar", [] {
            if (idLevelSaatIni() < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            if (tgm::bot::jumlahAksi() == 0) return beriTahu("Belum ada macro: rekam atau muat dulu", NotificationIcon::Warning);
            tgm::bot::mulaiPutar();
            beriTahu("Memutar: restart level untuk mulai dari awal", NotificationIcon::Success);
        }));
        r.push_back(tombol("Stop", [] { tgm::bot::berhenti(); beriTahu("Bot dihentikan"); }));
        r.push_back(stepInt("Slot Macro", c.slot, 1, 5, 1, [](int v) { return fmt::format("Slot {}", v); }));
        r.push_back(info([&c] {
            int id = idLevelSaatIni();
            if (id < 0) return std::string("Buka dari level untuk slot");
            auto i = tgm::bot::infoSlot(id, c.slot);
            if (!i.ada) return fmt::format("Slot {} kosong", c.slot);
            return fmt::format("Slot {}: {} aksi, {:.1f} dtk", c.slot, i.aksi, i.detik);
        }));
        r.push_back(tombol("Simpan", [&c] {
            int id = idLevelSaatIni();
            if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            std::string pesan;
            bool ok = tgm::bot::simpan(id, c.slot, pesan);
            beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
        }));
        r.push_back(tombol("Muat", [&c] {
            int id = idLevelSaatIni();
            if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            std::string pesan;
            bool ok = tgm::bot::muat(id, c.slot, pesan);
            beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
        }));
        r.push_back(tombol("Hapus", [&c] {
            int id = idLevelSaatIni();
            if (id < 0) return beriTahu("Buka dari dalam level", NotificationIcon::Warning);
            std::string pesan;
            bool ok = tgm::bot::hapus(id, c.slot, pesan);
            beriTahu(pesan, ok ? NotificationIcon::Success : NotificationIcon::Error);
        }));
        r.push_back(tog("Simpan Otomatis Saat Selesai", c.autoSaveMacro));
        r.push_back(tog("Putar Otomatis dari Awal", c.autoPlay));
        r.push_back(tog("Muat Macro Otomatis", c.autoMuat));
        tambah("Bot", std::move(r));
    }

    // ================= Labels =================
    {
        std::vector<Baris> r;
        r.push_back(tog("Tampilkan Label", c.showHud));
        r.push_back(tog("FPS", c.showFps));
        r.push_back(tog("CPS", c.showCps));
        r.push_back(tog("Jumlah Klik", c.showClicks));
        r.push_back(tog("Jam", c.showClock));
        r.push_back(tog("Persentase", c.showPercent));
        r.push_back(tog("Accurate Percentage", c.persenAkurat));
        r.push_back(tog("Percobaan", c.showAttempt));
        r.push_back(tog("Frame", c.showFrame));
        r.push_back(tog("Waktu Level", c.showTime));
        r.push_back(tog("Mati Selama Sesi", c.showDeaths));
        r.push_back(tog("Persen Terbaik", c.showBest));
        r.push_back(tog("Persen Mati Terakhir", c.showMatiTerakhir));
        r.push_back(tog("Nama Level", c.showLevelName));
        r.push_back(tog("Akurasi Noclip", c.showAkurasi));
        r.push_back(tog("Cheat Indicator", c.showCheat));
        r.push_back(tog("Testmode", c.showTestmode));
        r.push_back(tog("Best Run", c.showBestRun));
        r.push_back(tog("Noclip Mati", c.showNoclipMati));

        std::vector<Baris> gaya;
        gaya.push_back(stepInt("Kepekatan", c.hudOpacity, 10, 100, 10, [](int v) { return fmt::format("{}%", v); }));
        gaya.push_back(stepInt("Ukuran", c.hudScale, 50, 150, 10, [](int v) { return fmt::format("{}%", v); }));
        gaya.push_back(stepPilih("Posisi", c.hudCorner, NAMA_SUDUT));
        gaya.push_back(stepPilih("Warna", c.hudColor, NAMA_WARNA));
        gaya.push_back(stepPilih("Huruf", c.hudFont, NAMA_FONT));
        gaya.push_back(tog("Latar Gelap", c.hudPanel));
        gaya.push_back(stepInt("Geser X", c.hudOffX, -100, 100, 5));
        gaya.push_back(stepInt("Geser Y", c.hudOffY, -100, 100, 5));
        r.push_back(grup("Gaya Label", std::move(gaya)));
        tambah("Labels", std::move(r));
    }

    // ================= Input =================
    {
        std::vector<Baris> r;
        std::vector<Baris> iv;
        iv.push_back(stepPilih("Posisi", c.ivPosisi, NAMA_SUDUT));
        iv.push_back(stepInt("Ukuran", c.ivUkuran, 50, 200, 10, [](int v) { return fmt::format("{}%", v); }));
        iv.push_back(stepInt("Kepekatan", c.ivOpasitas, 20, 100, 10, [](int v) { return fmt::format("{}%", v); }));
        iv.push_back(stepPilih("Warna", c.ivWarna, NAMA_WARNA));
        iv.push_back(stepInt("Geser X", c.ivOffX, -150, 150, 5));
        iv.push_back(stepInt("Geser Y", c.ivOffY, -150, 150, 5));
        auto ivBaris = togAnak("Input Viewer", c.ivTampil, std::move(iv));
        ivBaris.terbuka = true;
        r.push_back(std::move(ivBaris));
        r.push_back(info("Mode Klik: lihat jendela Player"));
        tambah("Input", std::move(r));
    }

    // ================= Shortcuts =================
    {
        std::vector<Baris> r;
        r.push_back(tombol("Restart Level", [] {
            auto* pl = levelUntukAksi();
            if (!pl) return;
            Ref<PlayLayer> jaga = pl;
            MegaMenu::tutupJikaAda();
            auto* scene = CCDirector::get()->getRunningScene();
            if (scene && pl->m_isPaused) {
                if (auto* jeda = scene->getChildByType<PauseLayer>(0)) {
                    jeda->onRestart(nullptr);
                    return;
                }
            }
            pl->resetLevelFromStart();
        }));
        r.push_back(tombol("Toggle Practice Mode", [] {
            auto* pl = levelUntukAksi();
            if (!pl) return;
            pl->togglePracticeMode(!pl->m_isPracticeMode);
            beriTahu(pl->m_isPracticeMode ? "Practice mode aktif" : "Practice mode mati");
        }));
        r.push_back(tombol("Place Checkpoint", [] {
            auto* pl = levelUntukAksi();
            if (!pl) return;
            if (!pl->m_isPracticeMode) return beriTahu("Nyalakan practice mode dulu", NotificationIcon::Warning);
            pl->markCheckpoint();
            beriTahu("Checkpoint dipasang");
        }));
        r.push_back(tombol("Remove Checkpoint", [] {
            auto* pl = levelUntukAksi();
            if (!pl) return;
            if (!pl->m_isPracticeMode) return beriTahu("Nyalakan practice mode dulu", NotificationIcon::Warning);
            if (pl->m_checkpointArray && pl->m_checkpointArray->count() > 0) {
                pl->m_checkpointArray->removeLastObject();
            }
            beriTahu("Checkpoint terakhir dihapus");
        }));
        r.push_back(tombol("Tutup Menu", [] { MegaMenu::tutupJikaAda(); }));
        r.push_back(info("Folder data mod:"));
        r.push_back(info([] { return Mod::get()->getSaveDir().string(); }));
        tambah("Shortcuts", std::move(r));
    }

    // ================= Keybinds =================
    {
        std::vector<Baris> r;
        r.push_back(info("Tahan 0,6 dtk pada toggle"));
        r.push_back(info("atau tombol di menu, lalu"));
        r.push_back(info("tekan tombol keyboard."));
        r.push_back(info("Esc batal, Del hapus."));
        r.push_back(kunci("Buka Menu", tgm::kb::ID_MENU));
        r.push_back(kunci("P1 Lompat", tgm::kb::ID_P1));
        r.push_back(kunci("P2 Lompat", tgm::kb::ID_P2));
        r.push_back(judul("Keybind Kustom"));
        auto daftar = tgm::kb::daftarKustom();
        if (daftar.empty()) r.push_back(info("Belum ada keybind kustom"));
        for (auto const& d : daftar) r.push_back(kunci(d.label, d.id));
        r.push_back(tombol("Reset Keybind", [] {
            createQuickPopup("Reset Keybind", "Hapus semua keybind kustom dan kembalikan Buka Menu ke Tab?", "Batal", "Reset",
                [](auto*, bool ya) {
                    if (!ya) return;
                    tgm::kb::reset();
                    beriTahu("Keybind direset", NotificationIcon::Success);
                    MegaMenu::susunUlangJikaTerbuka();
                });
        }));
        r.push_back(info("Butuh keyboard (PC / Bluetooth)"));
        tambah("Keybinds", std::move(r));
    }

    // ================= Interface =================
    {
        std::vector<Baris> r;
        r.push_back(stepPilih("Tema", c.temaMenu, NAMA_TEMA));
        r.push_back(stepInt("Ukuran Menu", c.skalaUi, 50, 150, 10, [](int v) { return fmt::format("x{:.2f}", static_cast<float>(v) / 100.f); }));
        r.push_back(stepInt("Kepekatan Latar", c.opasitasMenu, 30, 100, 10, [](int v) { return fmt::format("{}%", v); }));
        r.push_back(tog("Menu Klasik (popup tab)", c.menuKlasik));
        r.push_back(info("Berlaku saat menu dibuka lagi"));

        std::vector<Baris> logo;
        logo.push_back(tog("Tampil di Level", c.logoDiLevel));
        logo.push_back(tog("Menempel ke Tepi", c.logoMagnet));
        logo.push_back(tog("Meredup Otomatis", c.logoRedup));
        logo.push_back(stepInt("Ukuran", c.logoUkuran, 50, 200, 10, [](int v) { return fmt::format("{}%", v); }));
        logo.push_back(stepInt("Kepekatan", c.logoOpasitas, 20, 100, 10, [](int v) { return fmt::format("{}%", v); }));
        logo.push_back(tombol("Reset Posisi", [] { FloatingLogo::resetPosisi(); beriTahu("Posisi tombol dikembalikan"); }));
        r.push_back(togAnak("Tombol Logo", c.logoTampil, std::move(logo)));
        tambah("Interface", std::move(r));
    }

    // ================= Statistik =================
    {
        std::vector<Baris> r;
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
            return fmt::format("Coba {} | Mati {} | Best {}%", l.percobaan, l.mati, l.terbaik);
        }));
        r.push_back(info([] {
            int id = idLevelSaatIni();
            if (id < 0) return std::string("");
            auto l = tgm::riwayat::level(id);
            return fmt::format("Klik {} | {} | Selesai {}x", l.klik, formatDurasi(l.detik), l.selesai);
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
        r.push_back(info([] { return fmt::format("Level tercatat: {}", tgm::riwayat::jumlahLevelTercatat()); }));
        tambah("Statistik", std::move(r));
    }

    // ================= Data =================
    {
        std::vector<Baris> r;
        r.push_back(info([] { return "Status: " + tgm::db::status(); }));
        r.push_back(info("Disimpan tiap ada perubahan"));
        r.push_back(tombol("Cadangkan", [] {
            bool ok = tgm::db::cadangkan();
            beriTahu(ok ? "Cadangan dibuat" : "Gagal membuat cadangan", ok ? NotificationIcon::Success : NotificationIcon::Error);
        }));
        r.push_back(tombol("Pulihkan", [] {
            createQuickPopup("Pulihkan", "Ganti data sekarang dengan cadangan manual terakhir?", "Batal", "Pulihkan",
                [](auto*, bool ya) {
                    if (!ya) return;
                    if (tgm::db::pulihkanDariCadangan()) {
                        tgm::loadConfig();
                        tgm::terapkanFps();
                        beriTahu("Data dipulihkan", NotificationIcon::Success);
                        MegaMenu::susunUlangJikaTerbuka();
                    } else {
                        beriTahu("Tidak ada cadangan manual", NotificationIcon::Warning);
                    }
                });
        }));
        r.push_back(tombol("Hapus Statistik", [] {
            createQuickPopup("Hapus Statistik", "Hapus semua statistik permanen? Pengaturan tidak ikut terhapus.", "Batal", "Hapus",
                [](auto*, bool ya) {
                    if (!ya) return;
                    tgm::riwayat::hapusSemua();
                    beriTahu("Statistik dihapus");
                });
        }));
        r.push_back(tombol("Reset Pengaturan", [] {
            createQuickPopup("Reset Pengaturan", "Kembalikan semua pengaturan ke bawaan?", "Batal", "Reset",
                [](auto*, bool ya) {
                    if (!ya) return;
                    tgm::resetConfig();
                    tgm::terapkanFps();
                    beriTahu("Pengaturan direset", NotificationIcon::Success);
                    MegaMenu::susunUlangJikaTerbuka();
                });
        }));
        r.push_back(judul("Peringatan"));
        r.push_back(info("Safe Mode mencegah progres tersimpan."));
        r.push_back(info("Jangan kirim rekor memakai cheat."));
        tambah("Data", std::move(r));
    }

    for (auto& j : hasil) tandaiKunci(j.judul, j.baris);
    return hasil;
}

void MegaMenu::susunJendela() {
    auto& c = tgm::cfg();
    m_jendela = bangunJendela();

    // Jendela yang dilipat terakhir kali diingat
    for (std::size_t i = 0; i < m_jendela.size() && i < 16; ++i) {
        m_jendela[i].tutup = ((c.mmTutup >> static_cast<int>(i)) & 1) != 0;
    }
}

void MegaMenu::tutupJikaAda() {
    auto* scene = CCDirector::get()->getRunningScene();
    if (!scene) return;
    if (auto* m = typeinfo_cast<MegaMenu*>(scene->getChildByID(ID_MENU))) m->tutup();
}

void MegaMenu::susunUlangJikaTerbuka() {
    auto* scene = CCDirector::get()->getRunningScene();
    if (!scene) return;
    if (auto* m = typeinfo_cast<MegaMenu*>(scene->getChildByID(ID_MENU))) m->susunUlangPertahankan();
}

// Susun ulang isi jendela (isi baris bisa berubah, mis. daftar keybind) tanpa kehilangan posisi gulir dan baris yang terbuka
void MegaMenu::susunUlangPertahankan() {
    if (m_menutup) return;
    std::vector<float> gulir;
    std::set<std::string> terbuka;
    for (auto const& j : m_jendela) {
        gulir.push_back(j.gulir);
        kumpulTerbuka(j.baris, terbuka);
    }
    this->susunJendela();
    for (std::size_t i = 0; i < m_jendela.size() && i < gulir.size(); ++i) {
        m_jendela[i].gulir = gulir[i];
        pulihTerbuka(m_jendela[i].baris, terbuka);
    }
    this->bangunUlang();
}

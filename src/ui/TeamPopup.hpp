#pragma once
// Menu utama TeamGdMod. Tampilan panel berkolom (header berwarna, tiap panel punya daftar sendiri yang bisa
// di-scroll bila kepanjangan). Isi panel disusun dari data (toggle, stepper, tombol, info), jadi menambah
// fitur cukup menambah baris di bangunPanel().

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>

#include <functional>
#include <string>
#include <utility>
#include <vector>

class TeamPopup : public geode::Popup {
public:
    struct Baris {
        enum class Jenis { Judul, Toggle, Stepper, Tombol, Info };
        Jenis jenis = Jenis::Info;
        std::string teks;
        std::function<bool()> ambil;                 // Toggle: nilai sekarang
        std::function<void(bool)> setel;             // Toggle: terapkan nilai
        std::function<std::string()> nilai;          // Stepper / Info: teks yang tampil
        std::function<void(int)> ubah;               // Stepper: arah -1 / +1
        std::vector<std::pair<std::string, std::function<void()>>> tombol;   // Tombol: 1 - 3 tombol
    };

    struct Panel {
        std::string nama;
        std::vector<Baris> baris;
    };

protected:
    struct Centang {                                 // kotak centang yang warnanya mengikuti nilai
        cocos2d::CCLayerColor* node = nullptr;
        std::function<bool()> ambil;
        cocos2d::ccColor3B hidup{};
        cocos2d::ccColor3B mati{};
    };
    struct Skrol {                                   // satu panel yang bisa di-scroll + penunjuk posisinya
        geode::ScrollLayer* layer = nullptr;
        cocos2d::CCLayerColor* bar = nullptr;
        float tinggiIsi = 0.f;
        float tinggiLihat = 0.f;
        float atasY = 0.f;
        float tinggiBar = 0.f;
        int panel = 0;
    };

    std::vector<Panel> m_panel;
    int   m_halaman = 0;
    int   m_perHalaman = 3;
    float m_skala = 1.f;
    bool  m_bangunUlang = false;
    bool  m_menutup = false;

    cocos2d::CCNode* m_isi = nullptr;                // wadah semua panel halaman aktif
    cocos2d::CCLabelBMFont* m_lblHalaman = nullptr;
    CCMenuItemSpriteExtra* m_panahKiri = nullptr;
    CCMenuItemSpriteExtra* m_panahKanan = nullptr;
    cocos2d::CCLayerColor* m_bgKiri = nullptr;
    cocos2d::CCLayerColor* m_bgKanan = nullptr;
    std::vector<Skrol> m_skrol;
    std::vector<Centang> m_centang;
    std::vector<std::pair<cocos2d::CCLabelBMFont*, std::function<std::string()>>> m_dinamis;

    bool init() override;
    void susun();                                    // bangun ulang semua panel halaman aktif
    void gantiHalaman(int arah);
    void segarkan(float dt);
    void segarkanCentang();
    void update(float dt) override;

    static std::vector<Panel> bangunPanel(TeamPopup* self);

public:
    void show() override;
    void onClose(cocos2d::CCObject* pengirim) override;
    static TeamPopup* create();
    // Buka menu bila tertutup, tutup bila sudah terbuka (dipakai tombol logo mengambang)
    static void bukaAtauTutup();
    // Minta susun ulang panel pada siklus berikutnya (aman dipanggil dari callback tombol)
    void mintaBangunUlang() { m_bangunUlang = true; }
};

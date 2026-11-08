#pragma once
// Menu utama TeamGdMod. Tab di sisi kiri, isi berupa baris-baris pengaturan yang disusun dari data
// (toggle, stepper, tombol, info) dan dibagi per halaman, jadi menambah fitur cukup menambah baris.

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

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
    using Halaman = std::vector<Baris>;

protected:
    static constexpr int JUMLAH_TAB = 10;
    static constexpr int BARIS_PER_HALAMAN = 8;

    std::vector<Halaman> m_halaman;                  // halaman tab aktif
    int m_tab = 0;
    int m_hal = 0;
    bool m_bangunUlang = false;

    CCMenuItemSpriteExtra* m_tombolTab[JUMLAH_TAB] = {};
    cocos2d::CCNode* m_layerIsi = nullptr;           // label baris
    cocos2d::CCMenu* m_menuIsi = nullptr;            // tombol baris
    cocos2d::CCLabelBMFont* m_lblHalaman = nullptr;
    CCMenuItemSpriteExtra* m_panahKiri = nullptr;
    CCMenuItemSpriteExtra* m_panahKanan = nullptr;
    std::vector<std::pair<cocos2d::CCLabelBMFont*, std::function<std::string()>>> m_dinamis;

    float m_skala = 1.f;                             // skala panel agar muat di layar sempit
    float m_arah = 1.f;                              // arah geser animasi ganti isi
    bool  m_sibuk = false;                           // animasi ganti isi sedang berjalan
    bool  m_menutup = false;                         // animasi tutup sedang berjalan
    std::function<void()> m_tertunda;                // pembangun isi baru yang menunggu animasi keluar

    bool init() override;
    void pilihTab(int tab, bool animasi = true);
    void tampilkanHalaman();
    void ganti(float arah, std::function<void()> bangun);
    void selesaiKeluar();
    void selesaiTutup();
    void segarkan(float dt);
    void gantiHalaman(int arah);

    static std::vector<Halaman> bangunTab(int tab, TeamPopup* self);

public:
    void show() override;
    void onClose(cocos2d::CCObject* pengirim) override;
    static TeamPopup* create();
    // Buka menu bila tertutup, tutup bila sudah terbuka (dipakai tombol logo mengambang)
    static void bukaAtauTutup();
    // Minta susun ulang halaman pada siklus berikutnya (aman dipanggil dari callback tombol)
    void mintaBangunUlang() { m_bangunUlang = true; }
};

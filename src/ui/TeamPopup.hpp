#pragma once
// Menu utama TeamGdMod (popup bertab): Hack, Noclip, HUD, Gaya HUD, Replay Bot, dan Info.

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

#include <array>
#include <vector>

class TeamPopup : public geode::Popup {
protected:
    static constexpr int JUMLAH_TAB = 6;

    // Node yang dimiliki tiap tab (ditampilkan/disembunyikan saat tab berganti)
    std::array<std::vector<cocos2d::CCNode*>, JUMLAH_TAB> m_isiTab;
    std::array<CCMenuItemSpriteExtra*, JUMLAH_TAB> m_tombolTab{};
    int m_tabAktif = 0;

    cocos2d::CCLabelBMFont* m_lblKecepatan = nullptr;
    cocos2d::CCLabelBMFont* m_lblCheckpoint = nullptr;
    cocos2d::CCLabelBMFont* m_lblOpasitas = nullptr;
    cocos2d::CCLabelBMFont* m_lblUkuran = nullptr;
    cocos2d::CCLabelBMFont* m_lblSudut = nullptr;
    cocos2d::CCLabelBMFont* m_lblWarna = nullptr;
    cocos2d::CCLabelBMFont* m_lblBatas = nullptr;
    cocos2d::CCLabelBMFont* m_lblSesi = nullptr;
    cocos2d::CCLabelBMFont* m_lblSlot = nullptr;
    cocos2d::CCLabelBMFont* m_lblStatus = nullptr;
    cocos2d::CCLabelBMFont* m_lblSlotTerisi = nullptr;

    bool init();

    // Pembantu penyusun UI. Semua node didaftarkan ke tab yang ditentukan.
    cocos2d::CCLabelBMFont* tambahLabel(int tab, char const* teks, float x, float y, float skala, char const* font = "bigFont.fnt", bool rataKiri = false);
    void tambahToggle(int tab, char const* teks, int tag, bool aktif, float y, float x = -160.f);
    CCMenuItemSpriteExtra* tambahTombol(int tab, char const* teks, cocos2d::SEL_MenuHandler aksi, float x, float y, float skala, int tag = 0);
    void tambahStepper(int tab, int tagMin, int tagPlus, float y, cocos2d::CCLabelBMFont*& label);

    void pilihTab(int tab);
    void segarkan(float dt);

    // Callback tombol
    void onTab(cocos2d::CCObject* pengirim);
    void onToggle(cocos2d::CCObject* pengirim);
    void onStep(cocos2d::CCObject* pengirim);
    void onPreset(cocos2d::CCObject* pengirim);
    void onRekam(cocos2d::CCObject*);
    void onPutar(cocos2d::CCObject*);
    void onBerhenti(cocos2d::CCObject*);
    void onSimpan(cocos2d::CCObject*);
    void onMuat(cocos2d::CCObject*);
    void onHapus(cocos2d::CCObject*);
    void onReset(cocos2d::CCObject*);

public:
    static TeamPopup* create();
};

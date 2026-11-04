#pragma once
// Menu utama TeamGdMod (popup): hack, kecepatan, dan kontrol Replay Bot.

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

class TeamPopup : public geode::Popup {
protected:
    cocos2d::CCLabelBMFont* m_labelKecepatan = nullptr;
    cocos2d::CCLabelBMFont* m_labelStatus    = nullptr;
    cocos2d::CCLabelBMFont* m_labelSlot      = nullptr;

    bool init();

    // Pembantu penyusun UI
    void tambahToggle(char const* teks, int tag, bool aktif, float y);
    void tambahTombol(char const* teks, cocos2d::SEL_MenuHandler aksi, float x, float y, float skala = 0.6f, int tag = 0);
    void segarkan(float dt);

    // Callback tombol
    void onToggle(cocos2d::CCObject* pengirim);
    void onKecepatan(cocos2d::CCObject* pengirim);
    void onSlot(cocos2d::CCObject* pengirim);
    void onRekam(cocos2d::CCObject*);
    void onPutar(cocos2d::CCObject*);
    void onBerhenti(cocos2d::CCObject*);
    void onSimpan(cocos2d::CCObject*);
    void onMuat(cocos2d::CCObject*);

public:
    static TeamPopup* create();
};

#pragma once
// Tombol logo TeamGd yang mengambang di atas semua layar (menu, level, layar jeda) dan bisa di-drag.
// Ditempelkan ke OverlayManager milik Geode sehingga bertahan saat pindah scene.
// Ketuk = buka / tutup menu TeamGdMod. Tahan lalu geser = pindahkan. Posisi tersimpan di database.

#include <Geode/Geode.hpp>

class FloatingLogo : public cocos2d::CCLayer {
protected:
    cocos2d::CCNode* m_gambar = nullptr;
    float m_x = 0.f, m_y = 0.f;        // posisi saat ini (piksel layar)
    float m_tx = 0.f, m_ty = 0.f;      // posisi tujuan (animasi menempel ke tepi)
    cocos2d::CCPoint m_awalSentuh;
    cocos2d::CCPoint m_awalPosisi;
    bool  m_menekan = false;
    bool  m_menyeret = false;
    bool  m_posisiSiap = false;
    float m_diam = 0.f;                // detik sejak sentuhan terakhir

    bool init() override;
    void update(float dt) override;
    void bangunGambar();
    float jari() const;                // jari-jari tombol dalam piksel layar
    void atasiBatas();
    bool kena(cocos2d::CCPoint const& titik) const;

    bool ccTouchBegan(cocos2d::CCTouch* sentuh, cocos2d::CCEvent* peristiwa) override;
    void ccTouchMoved(cocos2d::CCTouch* sentuh, cocos2d::CCEvent* peristiwa) override;
    void ccTouchEnded(cocos2d::CCTouch* sentuh, cocos2d::CCEvent* peristiwa) override;
    void ccTouchCancelled(cocos2d::CCTouch* sentuh, cocos2d::CCEvent* peristiwa) override;
    void registerWithTouchDispatcher() override;

public:
    // Pasang ke overlay (aman dipanggil berulang)
    static void pasang();
    // Kembalikan ke posisi bawaan (dipakai tombol Reset Posisi)
    static void resetPosisi();
};

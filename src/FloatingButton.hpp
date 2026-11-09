#pragma once
#include "UI.hpp"
#if __has_include(<Geode/ui/OverlayManager.hpp>)
#include <Geode/ui/OverlayManager.hpp>
#define TG_HAS_OVERLAY 1
#else
#define TG_HAS_OVERLAY 0
#endif

namespace tg {

// Satu-satunya instance; hidup lintas scene lewat OverlayManager (tidak duplikat).
class FloatingButton : public CCLayer {
    CCSprite* m_bg = nullptr;
    CCSprite* m_logo = nullptr;
    float m_size = 28.f;
    float m_moved = 0.f;
    bool m_active = false;
    static inline FloatingButton* s_inst = nullptr;

    float margin() const { return m_size / 2.f + 3.f; }

    bool init() override {
        if (!CCLayer::init()) return false;
        setAnchorPoint({0.5f, 0.5f});
        ignoreAnchorPointForPosition(false);
        m_bg = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
        m_bg->setColor({130, 30, 170});
        m_logo = CCSprite::create("logo_btn.png"_spr);
        addChild(m_bg);
        if (m_logo) addChild(m_logo, 1);
        setTouchMode(kCCTouchesOneByOne);
        setTouchPriority(-600);
        setTouchEnabled(true);
        setZOrder(10000);
        applySettings();
        auto win = CCDirector::get()->getWinSize();
        float fx = (float)Mod::get()->getSavedValue<double>("btn-fx", 0.04);
        float fy = (float)Mod::get()->getSavedValue<double>("btn-fy", 0.6);
        setPosition({fx * win.width, fy * win.height});
        clampToScreen();
        return true;
    }

    void clampToScreen() {
        auto win = CCDirector::get()->getWinSize();
        float m = margin();
        setPosition({std::clamp(getPositionX(), m, win.width - m),
                     std::clamp(getPositionY(), m, win.height - m)});
    }
    bool hit(CCTouch* t) { return ccpDistance(t->getLocation(), getPosition()) <= m_size / 2.f + 4.f; }

public:
    void applySettings() {
        m_size = (float)Mod::get()->getSettingValue<double>("button-size") * 0.6f;
        float op = (float)Mod::get()->getSettingValue<double>("button-opacity");
        setContentSize({m_size, m_size});
        m_bg->setScale(m_size / m_bg->getContentWidth());
        m_bg->setPosition({m_size / 2, m_size / 2});
        m_bg->setOpacity((GLubyte)(op * 255));
        if (m_logo) {
            m_logo->setScale((m_size * 0.72f) / m_logo->getContentWidth());
            m_logo->setPosition({m_size / 2, m_size / 2});
            m_logo->setOpacity((GLubyte)(op * 255));
        }
        if (getParent()) clampToScreen();
    }

    static void ensure() {
        if (s_inst) return;
        auto b = new FloatingButton();
        if (b && b->init()) {
            b->autorelease();
            s_inst = b;
#if TG_HAS_OVERLAY
            OverlayManager::get()->addChild(b);
#else
            attach();
#endif
        } else {
            CC_SAFE_DELETE(b);
        }
    }
    // cadangan bila OverlayManager tidak ada: tempel ulang ke scene aktif (tetap satu instance)
    static void attach() {
        if (!s_inst) return;
        auto sc = CCDirector::get()->getRunningScene();
        if (!sc || s_inst->getParent() == sc) return;
        s_inst->retain();
        s_inst->removeFromParent();
        sc->addChild(s_inst, 10000);
        s_inst->release();
    }
    static void refresh() { if (s_inst) s_inst->applySettings(); }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!isVisible() || !hit(t)) return false;
        m_active = true;
        m_moved = 0.f;
        return true;
    }
    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        if (!m_active) return;
        auto d = t->getLocation() - t->getPreviousLocation();
        m_moved += std::abs(d.x) + std::abs(d.y);
        if (m_moved < 6.f) return;
        setPosition(getPosition() + d);
        clampToScreen();
    }
    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_active) return;
        m_active = false;
        if (m_moved < 6.f) { toggleMenu(); return; }
        auto win = CCDirector::get()->getWinSize();
        float tx = getPositionX() < win.width / 2 ? margin() : win.width - margin(); // edge snapping
        runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.2f, {tx, getPositionY()})));
        Mod::get()->setSavedValue<double>("btn-fx", tx / win.width);
        Mod::get()->setSavedValue<double>("btn-fy", getPositionY() / win.height);
    }
    void ccTouchCancelled(CCTouch* t, CCEvent* e) override { ccTouchEnded(t, e); }
};

} // namespace tg

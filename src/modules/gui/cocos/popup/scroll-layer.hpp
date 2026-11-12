#pragma once

#include <functional>

namespace eclipse::gui::cocos {
    // taken from Object Workshop (a hybrid of robtops cocos and geode)
    class ScrollLayer : public CCScrollLayerExt, public CCScrollLayerExtDelegate {
    protected:
        bool m_touchOutOfBoundary;
        bool m_scrollWheelEnabled;
        cocos2d::CCTouch* m_touchStart;
        cocos2d::CCPoint m_touchStartPosition2;
        cocos2d::CCPoint m_touchPosition2;
        bool m_touchMoved;
        float m_touchLastY;
        bool m_cancellingTouches;
        std::function<void(cocos2d::CCPoint const&)> m_tapCallback; // posisi tap (world space)

        void cancelAndStoleTouch(cocos2d::CCTouch*, cocos2d::CCEvent*);
        void checkBoundaryOfContent(float);
        void claimTouch(cocos2d::CCTouch*);
        void touchFinish(cocos2d::CCTouch*);

        bool ccTouchBegan(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
        void ccTouchMoved(cocos2d::CCTouch*, cocos2d::CCEvent*) override;

        void ccTouchEnded(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
        void ccTouchCancelled(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
        bool scrollWheel(float, float) override;
        void visit() override;
        void registerWithTouchDispatcher() override;
        CCMenuItemSpriteExtra* itemForTouch(cocos2d::CCTouch*);

        ScrollLayer(cocos2d::CCRect const& rect, bool scrollWheelEnabled, bool vertical);
        ~ScrollLayer() override;

    public:
        void scrollToTop();
        /// Dipanggil saat tap (tanpa geser) di dalam layer; posisi dalam world space.
        void setTapCallback(std::function<void(cocos2d::CCPoint const&)> cb) { m_tapCallback = std::move(cb); }
        static ScrollLayer* create(cocos2d::CCRect const& rect, bool scrollWheelEnabled = true, bool vertical = true);
        static ScrollLayer* create(cocos2d::CCSize const& size, bool scrollWheelEnabled = true, bool vertical = true);
    };
}

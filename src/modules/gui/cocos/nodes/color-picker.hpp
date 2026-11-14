#pragma once
#include <memory>
#include <modules/gui/color.hpp>
#include <modules/gui/cocos/cocos.hpp>

namespace eclipse::gui::cocos {
    class ColorPicker : public CCMenuItemSpriteExtra {
    public:
        static ColorPicker* create(Color const& original, bool useAlpha, Function<void(Color)>&& callback) {
            auto ret = new ColorPicker();
            if (ret->init(original, useAlpha, std::forward<Function<void(Color)>>(callback))) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }

        void setColor(gui::Color const& color) {
            m_color = color;
            m_colorSprite->setColor(color.toCCColor3B());
        }

    protected:
        bool init(gui::Color const& original, bool useAlpha, Function<void(Color)> callback);
        void onClicked(CCObject*);
        void updateColor(cocos2d::ccColor4B const& color);

    protected:
        cocos2d::CCSprite* m_colorSprite = nullptr;
        Function<void(Color)> m_callback;
        gui::Color m_color;
        bool m_useAlpha = false;
        geode::ColorPickPopup* m_popup = nullptr;
    };
}

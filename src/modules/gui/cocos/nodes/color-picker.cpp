#include "color-picker.hpp"

namespace eclipse::gui::cocos {
    namespace {
        // Geode 6: ColorPickPopup::create(color, isRGBA) menjadi protected.
        // Struct ini hanya membuka akses ke fungsi static itu. Tidak pernah diinstansiasi,
        // jadi tidak ada vtable baru (menghindari masalah simbol textChanged di perangkat).
        struct ColorPickPopupAccess : geode::ColorPickPopup {
            static geode::ColorPickPopup* make(cocos2d::ccColor4B const& color, bool isRGBA) {
                return geode::ColorPickPopup::create(color, isRGBA);
            }
        };
    }

    bool ColorPicker::init(Color const& original, bool useAlpha, Function<void(Color)> callback) {
        m_useAlpha = useAlpha;
        m_callback = std::move(callback);
        m_color = original;

        m_colorSprite = ColorChannelSprite::create();
        m_colorSprite->setScale(0.65F);
        m_colorSprite->setID("color-sprite");
        m_colorSprite->setColor(original.toCCColor3B());

        return CCMenuItemSpriteExtra::init(m_colorSprite, m_colorSprite, this, menu_selector(ColorPicker::onClicked));
    }

    void ColorPicker::updateColor(cocos2d::ccColor4B const& color) {
        m_color = gui::Color(color);
        m_colorSprite->setColor(m_color.toCCColor3B());
        m_callback(m_color);
    }

    void ColorPicker::onClicked(CCObject*) {
        if (m_useAlpha) {
            m_popup = ColorPickPopupAccess::make(static_cast<cocos2d::ccColor4B>(m_color), true);
        } else {
            m_popup = ColorPickPopupAccess::make(geode::cocos::to4B(m_color.toCCColor3B()), true);
        }

        m_popup->setCallback([self = geode::WeakRef(this)](cocos2d::ccColor4B const& color) {
            if (auto s = self.lock()) {
                s->updateColor(color);
            }
        });
        m_popup->show();

        if (auto cocos = CocosRenderer::get()) {
            cocos->registerModal(m_popup);
        }
    }
}

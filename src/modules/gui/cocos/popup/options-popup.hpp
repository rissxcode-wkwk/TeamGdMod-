#pragma once
#include <memory>
#include <Geode/ui/Popup.hpp>

namespace eclipse::gui {
    class MenuTab;
}

namespace eclipse::gui::cocos {

    class ContentView;

    /// @brief A popup with a ContentView for displaying extra hack options.
    class OptionsPopup : public geode::Popup {
    protected:
        bool init(std::shared_ptr<MenuTab> const& tab);
        void onExit() override;

    public:
        static OptionsPopup* create(std::shared_ptr<MenuTab> const& tab);

    protected:
        // Geode 6: m_bgSprite sekarang NineSlice*, jadi background kita simpan terpisah
        cocos2d::extension::CCScale9Sprite* m_mainBG = nullptr;
        cocos2d::extension::CCScale9Sprite* m_contentBG = nullptr;
        ContentView* m_contentView = nullptr;
    };
}
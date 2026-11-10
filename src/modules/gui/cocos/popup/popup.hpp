#pragma once
#include <modules/gui/gui.hpp>
#include <Geode/ui/Popup.hpp>

#include "content-view.hpp"
#include "tab-menu.hpp"

namespace eclipse::gui::cocos {

    // Geode 6: geode::Popup bukan template lagi dan tidak punya setup(); inisialisasi lewat init().
    class Popup : public geode::Popup {
    protected:
        bool init(Tabs const& tabs);
        void onExit() override;

    public:
        static Popup* create(Tabs const& tabs);

        void setActiveTab(int idx) const;
        void loadTab(const std::shared_ptr<MenuTab>& tab) const;

        bool isAncestorOf(cocos2d::CCNode* node) const;

    private:
        TabMenu* m_tabMenu = nullptr;

        // Geode 6: m_bgSprite sekarang NineSlice*, jadi background kita simpan terpisah
        cocos2d::extension::CCScale9Sprite* m_mainBG = nullptr;
        cocos2d::extension::CCScale9Sprite* m_contentBG = nullptr;
        ContentView* m_contentMenu = nullptr;

        int m_activeTab = 0;
    };

}
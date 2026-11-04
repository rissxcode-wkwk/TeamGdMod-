// Tombol pembuka menu TeamGdMod di layar pause dan menu utama (cocok untuk HP).

#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include "../ui/TeamPopup.hpp"

using namespace geode::prelude;

namespace {
    // Membuat menu kecil berisi satu tombol "TeamGd" pada posisi tertentu
    CCMenu* buatTombolTeamGd(CCObject* target, SEL_MenuHandler aksi, CCPoint posisi) {
        auto spr = ButtonSprite::create("TeamGd", "goldFont.fnt", "GJ_button_01.png", 0.6f);
        spr->setScale(0.7f);
        auto tombol = CCMenuItemSpriteExtra::create(spr, target, aksi);
        tombol->setPosition(posisi);

        auto menu = CCMenu::create();
        menu->setID("tgm-menu"_spr);
        menu->setPosition({0.f, 0.f});
        menu->addChild(tombol);
        return menu;
    }
}

class $modify(TgmPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto win = CCDirector::get()->getWinSize();
        this->addChild(buatTombolTeamGd(
            this, menu_selector(TgmPauseLayer::onTeamGd), {win.width - 50.f, 26.f}
        ));
    }

    void onTeamGd(CCObject*) {
        TeamPopup::create()->show();
    }
};

class $modify(TgmMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto win = CCDirector::get()->getWinSize();
        this->addChild(buatTombolTeamGd(
            this, menu_selector(TgmMenuLayer::onTeamGd), {52.f, win.height - 22.f}
        ));
        return true;
    }

    void onTeamGd(CCObject*) {
        TeamPopup::create()->show();
    }
};

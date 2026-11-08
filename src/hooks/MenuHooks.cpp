// Tombol pembuka menu TeamGdMod selain tombol logo mengambang:
//  - layar jeda: selalu ada (cadangan bila tombol logo disembunyikan)
//  - menu utama: muncul hanya bila tombol logo dimatikan
//  - layar jeda: konfirmasi sebelum keluar level (opsional)

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include "../core/Config.hpp"
#include "../core/Stats.hpp"
#include "../ui/MegaMenu.hpp"

using namespace geode::prelude;

namespace {
    // Tombol berlogo; bila gambar gagal dimuat dipakai tombol bertulisan
    CCMenuItemSpriteExtra* tombolLogo(float ukuran, geode::Function<void(CCMenuItemSpriteExtra*)> aksi) {
        CCNode* spr = CCSprite::create("logo.png"_spr);
        if (spr) {
            spr->setScale(ukuran / std::max(1.f, spr->getContentSize().width));
        } else {
            auto* tulisan = ButtonSprite::create("TeamGd", "goldFont.fnt", "GJ_button_01.png", 0.6f);
            tulisan->setScale(0.7f);
            spr = tulisan;
        }
        return CCMenuItemExt::createSpriteExtra(spr, std::move(aksi));
    }
}

class $modify(TgmPauseLayer, PauseLayer) {
    struct Fields {
        bool sudahSetuju = false;   // pengguna sudah mengonfirmasi keluar
        bool sudahSetujuRestart = false;   // pengguna sudah mengonfirmasi restart
    };

    // Konfirmasi sebelum restart (opsional). Restart dari menu / keybind TeamGdMod tidak ditanya lagi.
    void onRestart(CCObject* pengirim) {
        if (!tgm::cfg().konfirmasiRestart || m_fields->sudahSetujuRestart || tgm::g_lewatRestart) {
            PauseLayer::onRestart(pengirim);
            return;
        }
        Ref<TgmPauseLayer> diri = this;
        createQuickPopup("Restart Level", "Restart level dari awal?", "Batal", "Restart",
            [diri, pengirim](auto*, bool ya) {
                if (!ya) return;
                diri->m_fields->sudahSetujuRestart = true;
                diri->onRestart(pengirim);
            });
    }

    // Konfirmasi sebelum keluar level (mencegah salah tekan di HP)
    void onQuit(CCObject* pengirim) {
        if (!tgm::cfg().konfirmasiKeluar || m_fields->sudahSetuju) {
            PauseLayer::onQuit(pengirim);
            return;
        }
        Ref<TgmPauseLayer> diri = this;
        createQuickPopup("Keluar Level", "Yakin ingin keluar dari level?", "Batal", "Keluar",
            [diri, pengirim](auto*, bool ya) {
                if (!ya) return;
                diri->m_fields->sudahSetuju = true;
                diri->onQuit(pengirim);
            });
    }

    void customSetup() {
        PauseLayer::customSetup();

        // Tombol logo mengambang sudah tampil di layar jeda: jangan buat tombol kedua (penyebab logo ganda)
        auto const& c = tgm::cfg();
        if (c.logoTampil && c.logoDiLevel) return;

        auto win = CCDirector::get()->getWinSize();
        auto* item = tombolLogo(34.f, [](auto) { MegaMenu::bukaAtauTutup(); });
        item->setPosition({win.width - 30.f, 30.f});
        auto* menu = CCMenu::create();
        menu->setID("tgm-menu"_spr);
        menu->setPosition({0.f, 0.f});
        menu->addChild(item);
        this->addChild(menu);
    }
};

class $modify(TgmMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        // Tombol logo mengambang sudah ada; tombol tetap di menu utama hanya sebagai cadangan
        if (!tgm::cfg().logoTampil) {
            auto win = CCDirector::get()->getWinSize();
            auto* item = tombolLogo(34.f, [](auto) { MegaMenu::bukaAtauTutup(); });
            item->setPosition({30.f, win.height - 30.f});
            auto* menu = CCMenu::create();
            menu->setID("tgm-menu"_spr);
            menu->setPosition({0.f, 0.f});
            menu->addChild(item);
            this->addChild(menu);
        }
        return true;
    }
};

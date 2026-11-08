// Memastikan database TeamGdMod tersimpan saat game keluar dari layar (HP) atau menyimpan datanya.

#include <Geode/Geode.hpp>
#include <Geode/modify/AppDelegate.hpp>

#include "../core/Db.hpp"

using namespace geode::prelude;

class $modify(TgmAppDelegate, AppDelegate) {
    void applicationDidEnterBackground() {
        // Game dikirim ke background: tulis semuanya sekarang, karena prosesnya bisa dimatikan sistem kapan saja
        tgm::db::flush();
        AppDelegate::applicationDidEnterBackground();
    }

    void trySaveGame(bool paksa) {
        tgm::db::flush();
        AppDelegate::trySaveGame(paksa);
    }
};

// Geode menyimpan data modnya: ikut flush database
$on_mod(DataSaved) {
    tgm::db::flush();
}

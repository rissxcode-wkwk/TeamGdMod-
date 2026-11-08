#pragma once
// Keybind kustom TeamGdMod.
//  - Tombol khusus: Buka Menu, P1 Lompat, P2 Lompat.
//  - Toggle atau tombol APA PUN di menu bisa dipasangi tombol keyboard: tahan barisnya 0,6 detik
//    (padanan "klik kanan") lalu tekan tombol yang diinginkan. Esc = batal, Del / Backspace = hapus.
//  - Semua keybind disimpan ke database (kunci "kb.data") dan dimuat saat game dibuka.
// Keybind butuh keyboard (komputer, atau keyboard Bluetooth di HP).

#include <functional>
#include <string>
#include <vector>

namespace tgm::kb {
    inline constexpr char const* ID_MENU = "@menu";
    inline constexpr char const* ID_P1   = "@p1";
    inline constexpr char const* ID_P2   = "@p2";

    struct Item {
        std::string id;      // "@menu" / "@p1" / "@p2" atau "<Jendela>/<Baris>"
        std::string label;   // nama yang ditampilkan
        int key = 0;         // kode tombol (cocos2d::enumKeyCodes)
    };

    void muat();                                           // dipanggil sekali saat data mod dimuat
    int  tombolDari(std::string const& id);                // 0 bila belum dipasang
    void pasang(std::string const& id, int key, std::string const& label);
    void hapus(std::string const& id);
    void reset();                                          // kembali ke bawaan (Buka Menu = Tab)
    std::vector<Item> daftarKustom();                      // keybind selain tombol khusus
    std::string namaKunci(int key);

    // Mode "tekan tombol": tombol berikutnya yang ditekan dipasang ke id
    void mulaiTangkap(std::string const& id, std::string const& label, std::function<void()> selesai);
    void batalTangkap();
    bool sedangMenangkap();
    bool sedangMenangkap(std::string const& id);

    // Dipanggil hook keyboard. true = tombol sudah ditangani (jangan diteruskan ke game)
    bool tangani(int key, bool down, bool repeat);
}

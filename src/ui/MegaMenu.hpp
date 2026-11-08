#pragma once
// Menu utama TeamGdMod bergaya jendela-jendela berjajar (seperti panel mod menu desktop):
// tiap jendela punya judul berwarna tema yang bisa dilipat, isinya baris-baris pengaturan.
//  - Geser jari ke kiri / kanan: menggulir deretan jendela.
//  - Geser jari ke atas / bawah di dalam jendela yang panjang: menggulir isi jendela itu.
//  - Ketuk judul jendela: lipat / buka. Ketuk baris: ubah pengaturan.
// Semua isi disusun dari data (toggle, stepper, tombol, info), jadi menambah fitur cukup menambah baris.

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <utility>
#include <vector>

class MegaMenu : public cocos2d::CCLayer {
public:
    struct Baris {
        enum class Jenis { Judul, Info, Toggle, Stepper, Tombol, Grup, Kunci };
        Jenis jenis = Jenis::Info;
        std::string teks;
        std::string kunciRow;                   // id baris ("Jendela/Teks"), dipakai keybind dan ingatan buka/tutup
        std::function<bool()> ambil;            // Toggle: nilai sekarang
        std::function<void(bool)> setel;        // Toggle: terapkan nilai
        std::function<std::string()> nilai;     // Stepper / Info: teks yang tampil
        std::function<void(int)> ubah;          // Stepper: arah -1 / +1
        std::function<void()> aksi;             // Tombol
        bool terbuka = false;                   // Toggle / Grup yang punya anak: anak sedang ditampilkan
        std::vector<Baris> anak;                // sub-pengaturan (muncul saat panah diketuk)
    };

    struct Jendela {
        std::string judul;
        std::vector<Baris> baris;
        float gulir = 0.f;                      // pergeseran isi ke atas (piksel logis)
        float tinggiIsi = 0.f;
        float tinggiBody = 0.f;
        bool tutup = false;                     // dilipat
    };

protected:
    struct Datar { Baris* baris; int tingkat; };
    struct Dinamis {
        cocos2d::CCLabelBMFont* label;
        std::function<std::string()> fn;
        float lebarMaks;
        float skala;
    };

    std::vector<Jendela> m_jendela;
    std::vector<cocos2d::CCNode*> m_akarJendela;   // node tiap jendela (untuk geser horizontal)
    std::vector<cocos2d::CCNode*> m_isiJendela;    // node isi tiap jendela (untuk geser vertikal), null bila dilipat
    std::vector<cocos2d::CCNode*> m_gagang;        // penanda posisi gulir tiap jendela
    std::vector<Dinamis> m_dinamis;

    cocos2d::CCLayerColor* m_dim = nullptr;
    cocos2d::CCNode* m_dunia = nullptr;            // akar semua gambar, diskalakan sesuai "Ukuran Menu"
    cocos2d::CCRect m_rectTutup;
    float m_lw = 0.f, m_lh = 0.f;                  // ukuran layar dalam satuan logis (sudah dibagi skala)
    float m_atas = 0.f;                            // tepi atas jendela
    float m_gulirX = 0.f;                          // pergeseran horizontal deretan jendela (<= 0)

    cocos2d::CCPoint m_mulai, m_akhir;             // sentuhan (satuan logis)
    bool m_geser = false;
    int  m_sumbu = 0;                              // 1 = horizontal, 2 = vertikal
    int  m_jendelaSentuh = -1;                     // jendela yang disentuh saat awal (body), -1 bila tidak ada
    bool m_menutup = false;
    bool m_tahanTerpicu = false;                   // tekan-tahan sudah memicu pemasangan keybind (abaikan ketukan)

    bool init() override;
    void susunJendela();
    void bangunUlang();
    void gambarBaris(cocos2d::CCNode* isi, Datar const& d, float y, int indeks);
    void aturPosisiX();
    void perbaruiGagang(std::size_t i);
    float gulirMaks(std::size_t i) const;
    float skala() const;
    cocos2d::CCPoint keLogis(cocos2d::CCTouch* sentuh) const;
    int cariJendelaBody(cocos2d::CCPoint p) const;
    void ketuk(cocos2d::CCPoint p);
    void aksiBaris(Baris& b, float x);
    void segarkan(float dt);
    void tahanSentuh(float dt);
    Baris* cariBaris(cocos2d::CCPoint p, float& xLokal);
    void susunUlangPertahankan();
    void hapus();

    bool ccTouchBegan(cocos2d::CCTouch* sentuh, cocos2d::CCEvent* peristiwa) override;
    void ccTouchMoved(cocos2d::CCTouch* sentuh, cocos2d::CCEvent* peristiwa) override;
    void ccTouchEnded(cocos2d::CCTouch* sentuh, cocos2d::CCEvent* peristiwa) override;
    void ccTouchCancelled(cocos2d::CCTouch* sentuh, cocos2d::CCEvent* peristiwa) override;
    void registerWithTouchDispatcher() override;
    void keyBackClicked() override;

public:
    static MegaMenu* create();
    // Buka menu bila tertutup, tutup bila sudah terbuka (dipakai tombol logo mengambang)
    static void bukaAtauTutup();
    static bool terbuka();
    // Isi semua jendela sebagai data (tanpa menu): dipakai menu dan sistem keybind
    static std::vector<Jendela> bangunJendela();
    static void tutupJikaAda();
    // Susun ulang isi menu bila sedang terbuka (mis. setelah keybind berubah)
    static void susunUlangJikaTerbuka();
    void tutup();
};

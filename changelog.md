# TeamGdMod

## v1.7.0
**Keybind kustom**:
* <cg>Ditambah</c> jendela <cl>Keybinds</c>: <co>Buka Menu</c> (bawaan Tab), <co>P1 Lompat</c>, <co>P2 Lompat</c>
* <cg>Ditambah</c> pasang tombol keyboard ke <cl>toggle atau tombol apa pun</c> di menu: tahan barisnya 0,6 detik, lalu tekan tombol (Esc batal, Del hapus). Keybind tersimpan di database
* Keybind butuh keyboard (komputer atau keyboard Bluetooth)

**Fitur baru**:
* <cg>Ditambah</c> <cl>Instant Mirror</c>, <cl>Auto Kill</c> (mati di persen pilihan), <cl>Mute Audio Saat Mati</c>
* <cg>Ditambah</c> tombol <cl>Speed +0.1 / -0.1</c> dan <cl>Stepper: Jeda/Lanjut</c>, <cl>Stepper: Langkah</c> (bisa dipasangi keybind)
* <cg>Ditambah</c> label: <cl>Testmode</c>, <cl>Best Run</c>, <cl>Noclip Mati</c>

## v1.6.0
**Tampilan**:
* <cy>Diubah</c> menu utama kini berupa <cl>jendela-jendela berjajar</c> (Global, Level, Player, Bypass, Bot, Labels, Input, Shortcuts, Interface, Statistik, Data). Geser kiri / kanan untuk menggulir deretan jendela, geser atas / bawah untuk menggulir isi jendela yang panjang, ketuk judul untuk melipat. Menu popup tab lama masih ada lewat <co>Menu Klasik</c>
* <cg>Ditambah</c> <cl>Tema menu</c> (8 warna), <cl>Ukuran Menu</c>, dan <cl>Kepekatan Latar</c> di jendela Interface

**Fitur baru**:
* <cg>Ditambah</c> <cl>Mode Ringan</c>: Low Detail Mode otomatis, tanpa efek mati dan guncangan, HUD diperbarui lebih jarang, animasi menu dimatikan
* <cg>Ditambah</c> <cl>FPS Bypass</c> dengan FPS target 30 - 360
* <cg>Ditambah</c> <cl>Jejak Lintasan</c> (panjang dan warna bisa diatur), <cl>Auto Practice Mode</c>, <cl>Auto LDM</c>, <cl>Bekukan Percobaan</c>, <cl>Safe Mode</c> manual, <cl>Cheat Indicator</c>
* <cg>Ditambah</c> jendela <cl>Shortcuts</c>: Restart Level, Toggle Practice Mode, Place / Remove Checkpoint

## v1.5.1
**Perbaikan**:
* <cg>Diperbaiki</c> <cl>Input Viewer P1/P2</c>: status tombol kini dibaca langsung dari objek pemain GD (`m_holdingButtons`), bukan dari parameter event, jadi P1 dan P2 tidak bisa tertukar. Pemain mati atau level dijeda otomatis dianggap dilepas (tidak ada status tersangkut)
* <cg>Diperbaiki</c> logo ganda: tombol logo di layar jeda tidak lagi dibuat bila tombol mengambang sudah tampil. Tombol mengambang hanya satu instance (salinan lain dibersihkan otomatis)
* <cg>Diperbaiki</c> node HUD / Input Viewer ganda saat level dimuat ulang
* <cg>Diperbaiki</c> panel menu terpotong di layar sempit (diskalakan otomatis)

**Fitur baru**:
* <cg>Ditambah</c> <cl>Hitbox</c> dan <co>Hitbox Saat Mati</c> (memakai debug draw bawaan GD)
* <cg>Ditambah</c> <cl>Langkah Frame</c>: tombol Jeda dan Langkah di layar
* <cg>Ditambah</c> geser horizontal / vertikal untuk Input Viewer

**Tampilan**:
* <cy>Diubah</c> animasi: menu buka dan tutup dengan pudar + skala, perpindahan tab dan halaman dengan geser + pudar, tekan tombol lebih ringan
* <cy>Diubah</c> panel latar tab dan isi, logo di sisi judul

**Performa**:
* <cy>Diubah</c> Input Viewer hanya menyentuh node bila ada yang berubah

## v1.5.0
**Fitur baru**:
* <cg>Ditambah</c> <cl>Musik Practice Sama dengan Normal</c>: di Practice, lagu level asli diputar dan sinkron dengan checkpoint
* <cg>Ditambah</c> <cl>Input Viewer</c>: kotak tombol P1/P2 (dan kiri/kanan di platformer) dengan posisi, ukuran, kepekatan, warna
* <cg>Ditambah</c> <cl>Presisi Klik</c>: <co>Klik di Langkah</c> dan <co>Click Between Frames</c> (eksperimental)
* <cg>Ditambah</c> <co>Klik Otomatis</c> (turbo) 5-30 per detik
* <cg>Ditambah</c> konfirmasi keluar level, tombol restart di layar, <co>persen mati terakhir</c>, dan sebaran tempat mati per level

**Perubahan**:
* <cr>Dihapus</c> pemutar Latihan Musik dari v1.4.0 (salah arti), diganti Musik Practice Sama dengan Normal
* <cy>Diubah</c> menu: tab baru <cl>Input</c>

## v1.4.0
**Fitur baru**:
* <cg>Ditambah</c> <cl>tombol logo mengambang</c> yang bisa di-drag, tampil di menu, level, dan layar jeda
* <cg>Ditambah</c> <cl>Latihan Musik</c>: lagu asli level tanpa membuka level, seek, kecepatan, volume, loop A-B
* <cg>Ditambah</c> <cl>database</c> tulis langsung (atomik, cadangan, checksum) untuk semua pengaturan dan statistik
* <cg>Ditambah</c> <co>Respawn Cepat</c>, <co>Lompat Bebas</c>, <co>Tanpa Guncangan</c>, <co>Tanpa Portal Cermin</c>, <co>Buka Semua Ikon</c>
* <cg>Ditambah</c> Noclip: <co>batas akurasi</c>; HUD: <co>CPS</c>, <co>persen terbaik</c>, <co>nama level</c>, <co>akurasi</c>
* <cg>Ditambah</c> gaya HUD: 6 posisi, 8 warna, 3 huruf, latar gelap, geser X/Y
* <cg>Ditambah</c> bot: <co>muat otomatis</c> dan info isi slot
* <cg>Ditambah</c> statistik permanen total dan per level

**Perubahan**:
* <cy>Diubah</c> menu: tab di sisi kiri, isi dibagi per halaman
* <cy>Diubah</c> pengaturan tidak lagi memakai saved value Geode (dimigrasikan otomatis)

## v1.3.0
**Dukungan versi baru**:
* <cg>Ditambah</c> dukungan <cl>Geometry Dash 2.2091</cl> (membutuhkan Geode <co>6.0.0-alpha.1</c> atau lebih baru)
* <cy>Diubah</c> penyimpanan macro mengikuti API path Geode 6
* Seluruh kode diuji kompilasi terhadap header Geode 6.0.0-alpha.1 dan bindings GD 2.2091

## v1.2.0
**Fitur baru**:
* <cg>Ditambah</c> tab <cl>Noclip</c>: berlaku per pemain (P1 / P2) dan <co>Batas Tabrakan</c>
* <cg>Ditambah</c> HUD: <co>persentase</c>, <co>percobaan</c>, <co>waktu level</c>, <co>frame</c>, dan <co>jumlah mati sesi</c>
* <cg>Ditambah</c> tab <cl>Gaya</c>: kepekatan, ukuran, posisi (4 sudut), dan warna HUD
* <cg>Ditambah</c> statistik sesi di tab Info (lama main, mati, klik)

## v1.1.0
**Fitur baru**:
* <cg>Ditambah</c> menu bertab: <cl>Hack</c>, <cl>HUD</c>, <cl>Bot</c>, <cl>Info</c>
* <cg>Ditambah</c> <co>Tanpa Efek Kematian</c> dan <co>Checkpoint Otomatis</c>
* <cg>Ditambah</c> HUD: <co>FPS</c>, <co>jumlah klik</c>, <co>jam</c>, dan kepekatan HUD
* <cg>Ditambah</c> preset kecepatan 0.5x, 1x, 2x
* <cg>Ditambah</c> bot: <co>hapus slot</c>, daftar slot terisi, durasi macro, <co>simpan otomatis</c>, <co>putar otomatis</c>
* <cg>Ditambah</c> tombol <co>Reset Pengaturan</c>

**Perbaikan**:
* <cy>Diperbaiki</c> error build pada `Cheats.hpp` (nama enum salah)
* <cy>Dirapikan</c> struktur kode dan seluruh kode diuji kompilasi terhadap header Geode 5.11.2

## v1.0.0
**Rilis pertama**:
* <cg>Ditambah</c> menu <cl>TeamGdMod</c>, <co>Noclip</c>, <co>Speedhack</c>, <co>Mode Aman</c>, HUD, dan <co>Replay Bot</c>

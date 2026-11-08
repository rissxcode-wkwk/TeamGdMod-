# TeamGdMod

## v1.6.0
**Tampilan**:
* <cy>Diubah</c> menu jadi <cl>panel berkolom</c> dengan header berwarna (Global, Level, Player, Bypass, Bot, Label, Gaya Label, Input Viewer, Interface, Shortcuts, Statistik, Data). Panel yang kepanjangan bisa di-scroll, panel bisa dilipat lewat header, dan halaman panel diganti lewat tombol < >
* <cg>Ditambah</c> <cl>Tema menu</c>: 8 warna aksen, <co>Mode Terang</c>, kepekatan panel, dan ukuran menu

**Fitur baru**:
* <cg>Ditambah</c> <cl>Mode Ringan</c>: partikel, guncangan, efek kematian dimatikan, HUD diperbarui lebih jarang, animasi menu dimatikan
* <cg>Ditambah</c> <co>Tanpa Partikel</c>, <co>Auto Practice Mode</c>
* <cg>Ditambah</c> <cl>Tampilkan Lintasan</c> (eksperimental): perkiraan busur lompatan kubus
* <cg>Ditambah</c> <cl>FPS Bypass</c> (eksperimental) dengan target FPS pilihan
* <cg>Ditambah</c> panel <cl>Shortcuts</c>: Restart Level, Toggle Practice, Pasang / Hapus Checkpoint

**Catatan**:
* Physics Bypass (TPS) belum ada: langkah fisika GD 2.2 harus diubah dari dalam game dan belum bisa diuji di sini
* Semua kode baru belum dikompilasi atau diuji di dalam game

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

# TeamGdMod

Mod menu Geometry Dash buatan sendiri (kode ditulis dari nol, **bukan** turunan Eclipse Menu).
Target: **Geode 6.0.0-alpha.1**, **GD 2.2091**, build Android 64-bit lewat GitHub Actions (platform lain bisa ditambah di workflow).

> **Penting:** Geode 6 masih berstatus **alpha**, dan GD 2.2091 hanya didukung resmi oleh Geode 6. Mod ini TIDAK akan
> termuat di game 2.2081 atau Geode 5. Untuk game 2.2081, gunakan TeamGdMod v1.2.0.

> **Status pengujian:** seluruh kode dikompilasi (cek sintaks dan tipe) dengan clang terhadap header
> Geode 6.0.0-alpha.1 dan bindings GD 2.2091 (Android64 disimulasikan), tanpa error. Yang **belum** dilakukan:
> link penuh dengan NDK dan uji di dalam game. Jadi perilaku saat bermain belum terbukti, terutama fitur
> **eksperimental**: Respawn Cepat, Lompat Bebas, Tanpa Portal Cermin, Presisi Klik (CBF), Musik Practice
> Sama dengan Normal, serta tombol logo mengambang (sentuhan dan posisi).

## Fitur

| Fitur | Keterangan |
|---|---|
| Tombol logo mengambang | Logo TeamGd bisa di-drag ke mana saja dan tampil di menu, level, dan layar jeda. Ketuk untuk buka menu. Menempel ke tepi, meredup otomatis, ukuran dan kepekatan diatur, posisi tersimpan |
| Musik Practice Sama dengan Normal | Di mode Practice, lagu level asli tetap diputar (sinkron dengan checkpoint) alih-alih musik practice terpisah bawaan GD |
| Input Viewer | Kotak tombol di layar yang menyala saat ditekan (P1 dan P2, juga kiri/kanan di level platformer). Posisi, ukuran, kepekatan, dan warna diatur |
| Presisi Klik | Klik di Langkah dan Click Between Frames (eksperimental): input diterapkan sesuai waktu sentuhan di dalam frame. Mati sendiri bila stempel waktu tidak cocok |
| Klik Otomatis | Tahan tombol lompat untuk spam klik 5-30 per detik (dihitung cheat) |
| Tambahan | Konfirmasi sebelum keluar level, tombol restart di layar, persen mati terakhir, dan sebaran tempat mati per level |
| Database | Semua pengaturan dan statistik ditulis langsung ke disk (atomik, ada cadangan dan checksum), jadi tidak hilang walau game ditutup paksa atau keluar level |
| Noclip | Per pemain (P1 / P2), Batas Tabrakan, dan mati bila akurasi di bawah ambang |
| Speedhack | 0.1x sampai 3.0x, hanya berlaku di dalam level |
| Mode Aman otomatis | Bila ada cheat aktif, level ditandai test mode sehingga progres tidak tersimpan |
| Respawn Cepat | Jeda respawn 0-1000 ms (eksperimental) |
| Lompat Bebas | Bisa lompat di udara (eksperimental) |
| Tanpa Guncangan / Tanpa Portal Cermin | Efek visual dimatikan |
| Buka Semua Ikon dan Warna | Hanya tampilan lokal |
| Tanpa Efek Kematian, Checkpoint Otomatis | Seperti versi sebelumnya |
| HUD | FPS, CPS, klik, jam, persen, persen terbaik, nama level, percobaan, waktu, frame, mati sesi, akurasi Noclip, status cheat dan bot |
| Gaya HUD | Kepekatan, ukuran, 6 posisi, 8 warna, 3 huruf, latar gelap, geser X/Y |
| Replay Bot | Rekam, putar, simpan, muat, hapus. 5 slot per level, info isi slot, simpan otomatis, putar otomatis, muat otomatis saat level dibuka |
| Statistik permanen | Total waktu main, mati, klik, percobaan, level selesai, dan data per level |
| Menu | Tab di kiri (Hack, Noclip, HUD, Gaya HUD, Input, Bot, Musik, Tombol, Statistik, Data), isi dibagi per halaman |

### Cara pakai Replay Bot
1. Masuk level, buka menu lewat tombol logo, pilih tab **Bot**, tekan **Rekam**, lalu **restart level** dan mainkan.
2. Saat respawn dari checkpoint, rekaman otomatis dipotong ke frame checkpoint itu.
3. Selesai (atau keluar level): buka menu lagi, pilih slot, tekan **Simpan**.
4. Untuk memutar: **Muat** slot, tekan **Putar**, lalu **restart level**. Input manual diabaikan selama pemutaran.

File macro tersimpan di folder simpanan mod: `macros/level<ID>_slot<N>.tgm`.
Database tersimpan di `database/teamgd.tgdb` (dengan `.bak`); tab **Data** punya Cadangkan dan Pulihkan.

**Keterbatasan:** level lokal/editor semua ber-ID 0 sehingga slot dan statistiknya berbagi. Macro mengandalkan
fisika yang deterministik pada tick rate yang sama. Presisi Klik bergantung pada stempel waktu input dari GD; bila tidak cocok fitur mati sendiri dan GD memakai penanganan bawaan. Presisi Klik otomatis mati saat Replay Bot merekam atau memutar.

## Struktur proyek

```
TeamGdMod/
├── mod.json                      metadata mod
├── CMakeLists.txt                konfigurasi build (semua src/**/*.cpp otomatis ikut)
├── resources/logo.png            logo bulat untuk tombol mengambang
├── .github/workflows/main.yml    build otomatis Android64
└── src/
    ├── core/     Db (database), Config (pengaturan), Stats, Riwayat (statistik permanen), Cheats
    ├── bot/      Replay Bot: data macro + file (.tgm), tanpa tipe GD
    ├── hacks/    Speedhack, NoDeathEffect, Unlocks, Visual, Cbf (presisi klik + Lompat Bebas)
    ├── hooks/    BotHooks, PlayLayerHooks, MenuHooks, SaveHooks
    └── ui/       TeamPopup (menu), FloatingLogo (tombol logo)
```

## Cara build (tanpa komputer, lewat GitHub Actions)

1. Unggah seluruh isi folder ini ke repo GitHub (termasuk folder tersembunyi `.github`).
2. Buka tab **Actions**, jalankan workflow **Build TeamGdMod** (otomatis jalan tiap push).
3. Setelah selesai, unduh artifact **TeamGdMod-android**, ekstrak, lalu salin file `.geode` ke folder `geode/mods` di game.
4. Kalau job merah, buka langkah **Build mod pakai Geode SDK** dan cari baris yang mengandung `error:`.

## Catatan

- Aturan Geode: mod yang ditulis AI tidak diterima di Geode Index resmi. Bagikan lewat repo GitHub sendiri.
- Fitur cheat sebaiknya tidak dipakai untuk mengirim rekor ke leaderboard. Karena itu Mode Aman aktif secara default.
- Lisensi: MIT (lihat `LICENSE`).

## Lisensi pihak ketiga

Logika Presisi Klik mengikuti pendekatan mod *Click Between Frames* (MIT). Lihat `THIRD_PARTY.md`.

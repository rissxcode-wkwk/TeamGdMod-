# TeamGdMod

Mod menu Geometry Dash buatan sendiri (kode ditulis dari nol, **bukan** turunan Eclipse Menu).
Target: **Geode 6.0.0-alpha.1**, **GD 2.2091**, build Android 64-bit lewat GitHub Actions (platform lain bisa ditambah di workflow).

> **Penting:** Geode 6 masih berstatus **alpha**, dan GD 2.2091 hanya didukung resmi oleh Geode 6. Mod ini TIDAK akan
> termuat di game 2.2081 atau Geode 5. Untuk game 2.2081, gunakan TeamGdMod v1.2.0.

> **Status pengujian:** seluruh kode dikompilasi (cek sintaks dan tipe) dengan clang terhadap header
> Geode 6.0.0-alpha.1 dan bindings GD 2.2091 untuk Android64, tanpa error. Logika Replay Bot (`src/bot`)
> juga lolos tes unit terpisah. Yang **belum** dilakukan: link penuh dengan NDK dan uji di dalam game.
> Jadi perilaku saat bermain (misalnya Mode Aman dan sinkronisasi bot saat respawn) belum terbukti.

## Fitur

| Fitur | Keterangan |
|---|---|
| Noclip | Tidak mati saat menabrak. Bisa per pemain (P1 / P2), dengan penghitung dan Batas Tabrakan |
| Speedhack | 0.1x sampai 3.0x, preset 0.5x / 1x / 2x, hanya berlaku di dalam level |
| Mode Aman otomatis | Bila ada cheat aktif, level ditandai test mode sehingga progres tidak tersimpan |
| Tanpa Efek Kematian | Melewati animasi ledakan saat mati |
| Checkpoint Otomatis | Di mode practice, jeda 1 sampai 30 detik |
| HUD | FPS, klik, jam, persen, percobaan, waktu, frame, mati sesi, status cheat dan bot |
| Gaya HUD | Kepekatan, ukuran, posisi (4 sudut), dan warna (5 pilihan) |
| Replay Bot | Rekam, putar, simpan, muat, hapus. 5 slot per level, simpan otomatis saat selesai, putar otomatis dari awal level |
| Menu bertab | Hack, Noclip, HUD, Gaya, Bot, Info, plus statistik sesi dan Reset Pengaturan |

### Cara pakai Replay Bot
1. Masuk level, pause, buka **TeamGd → Bot**, tekan **Rekam**, lalu **restart level** dan mainkan.
2. Saat respawn dari checkpoint, rekaman otomatis dipotong ke frame checkpoint itu.
3. Selesai (atau keluar level): buka menu lagi, pilih slot, tekan **Simpan**.
   Atau nyalakan **Simpan Otomatis Saat Selesai** agar tersimpan sendiri saat level tamat.
4. Untuk memutar: **Muat** slot, tekan **Putar**, lalu **restart level**. Input manual diabaikan selama pemutaran.
   Nyalakan **Putar Otomatis dari Awal** bila ingin macro langsung jalan tiap level dimulai dari awal.

File macro tersimpan di folder simpanan mod: `macros/level<ID>_slot<N>.tgm`.

**Keterbatasan:** level lokal/editor semua ber-ID 0 sehingga slotnya berbagi. Macro mengandalkan
fisika yang deterministik pada tick rate yang sama. Menekan ulang tombol yang sedang ditahan saat
respawn masih eksperimental. Durasi macro di menu dihitung dengan asumsi 240 frame fisika per detik.

## Struktur proyek

```
TeamGdMod/
├── mod.json                      metadata mod
├── CMakeLists.txt                konfigurasi build (semua src/**/*.cpp otomatis ikut)
├── .github/workflows/main.yml    build otomatis Android64
└── src/
    ├── core/     Config (pengaturan + penyimpanan), Stats (statistik sesi), Cheats (penentu cheat aktif)
    ├── bot/      Replay Bot: data macro + file (.tgm), tanpa tipe GD
    ├── hacks/    Speedhack, NoDeathEffect
    ├── hooks/    BotHooks (input), PlayLayerHooks (noclip, mode aman, HUD, checkpoint), MenuHooks (tombol)
    └── ui/       TeamPopup (menu bertab)
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

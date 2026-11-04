# TeamGdMod

Mod menu Geometry Dash buatan sendiri (kode ditulis dari nol, **bukan** turunan Eclipse Menu).
Target: **Geode 5.9.0**, **GD 2.2081**, platform Android / Windows / macOS / iOS.

> **Status:** kode ini belum pernah dikompilasi atau dites di game. Logika Replay Bot (`src/bot`)
> sudah diuji terpisah, tetapi bagian yang menyentuh GD (hook dan UI) baru dicocokkan dengan
> header Geode 5.9.0 dan bindings GD 2.2081 secara manual. Kemungkinan ada error build kecil.
> Kalau build gagal, kirim log error-nya dan kita perbaiki bersama.

## Fitur

| Fitur | Keterangan |
|---|---|
| Noclip | Tidak mati saat menabrak, HUD menghitung jumlah tabrakan |
| Speedhack | 0.1x sampai 3.0x, hanya berlaku di dalam level |
| Mode Aman otomatis | Bila ada cheat aktif, level ditandai test mode sehingga progres tidak tersimpan |
| HUD status | Label di pojok kiri atas: Noclip, kecepatan, status bot, Mode Aman |
| Replay Bot | Rekam, putar, simpan, muat. 5 slot per level, input dicatat per frame fisika |

### Cara pakai Replay Bot
1. Masuk level, pause, buka **TeamGd**, tekan **Rekam**, lalu **restart level** dan mainkan.
2. Saat respawn dari checkpoint, rekaman otomatis dipotong ke frame checkpoint itu.
3. Selesai (atau keluar level): buka menu lagi, pilih slot, tekan **Simpan**.
4. Untuk memutar: **Muat** slot, tekan **Putar**, lalu **restart level**. Input manual diabaikan selama pemutaran.

File macro tersimpan di folder simpanan mod: `macros/level<ID>_slot<N>.tgm`.

**Keterbatasan:** level lokal/editor semua ber-ID 0 sehingga slotnya berbagi. Macro mengandalkan
fisika yang deterministik pada tick rate yang sama. Pemulihan tombol yang sedang ditahan saat
respawn masih eksperimental.

## Struktur proyek

```
TeamGdMod/
├── mod.json                  metadata mod
├── CMakeLists.txt            konfigurasi build
├── .github/workflows/build.yml   build otomatis semua platform
└── src/
    ├── core/     Config (pengaturan + penyimpanan), Cheats (penentu cheat aktif)
    ├── bot/      Replay Bot: data macro + file (.tgm), tanpa tipe GD
    ├── hacks/    Speedhack
    ├── hooks/    BotHooks (input), PlayLayerHooks (noclip, mode aman, HUD), MenuHooks (tombol)
    └── ui/       TeamPopup (menu utama)
```

## Cara build (tanpa komputer, lewat GitHub Actions)

1. Buat repo baru di GitHub, lalu unggah seluruh isi folder ini dengan struktur yang sama.
   Di HP bisa lewat Termux (`git push`), aplikasi MGit, atau editor web GitHub
   (*Add file → Create new file*, ketik path seperti `src/bot/Bot.cpp` agar folder otomatis dibuat).
2. Buka tab **Actions**, jalankan workflow **Build TeamGdMod** (otomatis jalan tiap push).
3. Setelah selesai, unduh artifact **Build Output**, lalu salin file `.geode` ke folder `geode/mods` di game.
4. Kalau ada job yang merah, buka log job tersebut dan kirim pesan error-nya.

## Catatan

- Aturan Geode: mod yang ditulis AI tidak diterima di Geode Index resmi. Bagikan lewat repo GitHub sendiri.
- Fitur cheat sebaiknya tidak dipakai untuk mengirim rekor ke leaderboard. Karena itu Mode Aman aktif secara default.
- Lisensi: MIT (lihat `LICENSE`).

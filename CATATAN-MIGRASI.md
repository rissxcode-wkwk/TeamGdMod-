# Catatan migrasi: GD 2.2091 + Geode 6

Target: Geometry Dash **2.2091**, Geode **6.0.0-alpha.1** (otomatis kompatibel dengan 6.0.0 stabil).
Kalau Geode 6.0.0 stabil sudah rilis, ubah `"geode"` di `mod.json` menjadi `"6.0.0"` lalu push.
Workflow memakai `sdk: given`, jadi versi SDK mengikuti `mod.json`.

## Yang diubah
| Area | Perubahan |
|------|-----------|
| `mod.json` | geode `6.0.0-alpha.1`, gd `2.2091` semua platform, versi `v1.0.0-beta.5` |
| Hook | `PlayLayer::init(level, dontCreateObjects)` (2 parameter), `GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick)`, `GameToolbox::intToShortString(int, bool)` |
| Tipe | `gd::string/vector/map/...` -> `std::...` |
| Filesystem | `Mod::getSaveDir()` dkk sekarang `asp::fs::Path`; dijembatani lewat `src/modules/utils/compat.hpp` |
| File picker | `geode::Task` dihapus di v6, diganti `geode::async::spawn(pick(...))` |
| `ScrollLayer::scrollWheel` | sekarang mengembalikan `bool` |
| ImGui | 1.92.4 (`PushFont(font, size)`, `FontScaleMain`) |
| Dependensi | repo `EclipseMenu/imgui`, `rift`, `discord-rpc`, `bindings` sudah tidak bisa diunduh: diganti ImGui resmi, discord-rpc resmi, bindings resmi `geode-sdk/bindings`, dan **RIFT lokal** di `third_party/rift` |
| Include | `std-includes.hpp` di-include paksa lewat CMake (Geode v6 mengurangi include transitif) |

## Catatan penting
- Kode **belum dikompilasi** di sini (SDK Geode butuh toolchain Windows/Android). Hook sudah dicocokkan
  satu per satu dengan bindings 2.2091, dan RIFT lokal sudah dites. Kalau CI masih error, kirim lognya.
- RIFT lokal mendukung sintaks di `LABELS.md` (variabel, operator, ternary, fungsi bawaan, `cfg()`).
- Android32 tetap ada di workflow; kalau gagal, hapus entri `Android32` dari matrix.

## Penghapusan fitur ffmpeg
- Fitur **Internal Recorder** (satu-satunya yang butuh mod `eclipse.ffmpeg-api`) dihapus total:
  folder `src/modules/recorder/` dan `src/hacks/Recorder/`, serta tab "Internal Recorder" di menu.
- Blok `"dependencies": { "eclipse.ffmpeg-api": ... }` dihapus dari `mod.json`, jadi mod ini bisa
  dimuat tanpa mod ffmpeg terpasang.

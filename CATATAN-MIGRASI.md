# Catatan: EclipseMenu-main (v1.9.4) untuk GD 2.2091 + Geode 6.0.0-alpha.1

Seluruh source diganti dengan `EclipseMenu-main.zip`. `mod.json` bawaan source ini sudah menargetkan
Geometry Dash **2.2091** dan Geode **6.0.0-alpha.1** (semua platform), jadi tidak ada perubahan target.

## Yang diubah dari source asli
- **Fitur ffmpeg dihapus total**: `src/modules/recorder/`, `src/hacks/Recorder/`, `src/ffmpeg-api/`,
  tab "Recorder" di menu (ImGui dan Cocos), dependensi `eclipse.ffmpeg-api` di `mod.json`,
  dan `EXTERNALS eclipse.ffmpeg-api:1.2.0` di `CMakeLists.txt`.
- **Workflow**: `android64.yml` (GitHub Actions, sdk `given`, bindings dikunci ke commit
  `5db3b324c53cd8124efa7453131252bb78362c28`) dipakai lagi. Workflow bawaan upstream
  (`multi-platform.yml`, `publish.yml`) memakai runner Forgejo/Codeberg dan dinonaktifkan (`.disabled`).
- README/about: baris Internal Recorder dihapus.

## Catatan penting
- Kode **belum dikompilasi** di sini. Kalau CI error, kirim lognya.
- Source ini mengunduh beberapa dependensi dari `git.eclipse.menu` (imgui, rift, discord-presence,
  translations). Server itu tidak bisa dijangkau dari lingkungan saya, jadi belum terverifikasi.
  Kalau CI gagal di tahap "CPM/FetchContent", kirim lognya.

## Perbaikan build (11 Nov 2026)
- `value_or({})` pada hasil `filename()` / `stem()` (asp::fs) gagal dikompilasi di NDK: tipe template tidak bisa
  disimpulkan dari `{}`. Diganti helper `eclipse::utils::orEmpty(...)` di `src/modules/utils/optional-helpers.hpp`.
  File: i18n/translations.cpp, hacks/Bot/Bot.cpp, gui/imgui/imgui.cpp, gui/imgui/components/theme.cpp,
  gui/cocos/components/DirectoryComboComponent.hpp.
- `std::ranges::find(items, value)` di DirectoryComboComponent.hpp (vector<Path> vs PathView) tidak bisa dikompilasi;
  diganti `find_if` yang membandingkan `item.view() == value` (PathView == PathView; `PathView::view()` mengembalikan string_view sehingga ambigu).
- Bindings 2.2091 tidak punya `PlayLayer::m_currentCheckpoint`, `PlayLayer::setStartPosObject`, dan
  `GJGameState::m_unkPoint34`. Kode dibuat adaptif dengan `if constexpr (requires {...})`:
  - StartPosSwitcher.cpp: checkpoint hanya di-null kalau member-nya ada; start pos diset lewat `setStartPosObject`
    kalau ada, kalau tidak langsung ke `m_startPosObject`.
  - NoEndShake.cpp: `m_unkPoint34` hanya disimpan/dipulihkan kalau field-nya ada.
  Perilaku kedua fitur ini belum teruji di game; kalau tidak jalan, perlu nama field baru dari bindings 2.2091.
- `GJBaseGameLayer::m_allowedButtons` tidak ada di bindings 2.2091 (yang ada `m_queuedButtons`, artinya beda).
  Diganti helper `eclipse::utils::clearAllowedButtons(this)` yang hanya mengosongkan kalau member-nya ada
  (SwiftClick.cpp dan Bot.cpp). Kalau tidak ada, SwiftClick praktis tidak melakukan apa-apa; perlu dicek lagi di game.

## Update: fitur ffmpeg (Recorder) DIKEMBALIKAN
- Folder `src/modules/recorder`, `src/hacks/Recorder`, `src/ffmpeg-api` dan tab Recorder (gui.cpp, tab-menu.cpp) dipulihkan dari source asli.
- `EXTERNALS eclipse.ffmpeg-api:1.2.0` dihapus dari CMakeLists.txt. Recorder hanya memakai header lokal `src/ffmpeg-api/events.hpp`
  (komunikasi lewat Geode event), jadi tidak perlu mengunduh mod ffmpeg-api saat build.
- Dependensi `eclipse.ffmpeg-api` di mod.json tetap opsional. Untuk merekam, pasang mod `eclipse.ffmpeg-api` versi Geode 6
  (zip terpisah: ffmpeg-api-Geode6.zip, v2.1.0) di game, bersama Eclipse.
- Semua perbaikan build di atas (orEmpty, find_if, adaptif bindings) tetap berlaku.

## Perbaikan Cheat Indicator (11 Nov 2026)
- Posisi: default alignment di pembuatan label (0 = kiri atas) berbeda dengan default combo di menu (2 = kanan atas),
  jadi menu menampilkan satu posisi tapi indikator muncul di posisi lain. Sekarang keduanya 2 dan nilai dibatasi 0..8.
- Tidak terlihat: nilai bawaan lama (skala 0.5, opasitas 0.35) membuat titik "." hampir tak terlihat di HP. Default baru
  skala 1.5 / opasitas 0.8; pengguna yang masih memakai nilai bawaan lama dimigrasi otomatis sekali
  (flag `labels.cheat-indicator.migrated`). Rentang skala dinaikkan sampai 5. Opasitas kini langsung ter-update.
- Kunci terjemahan combo posisi disamakan dengan translations.cpp (`middle-left/center/right`, sebelumnya `center-*`).
- File: src/hacks/Labels/Labels.cpp, src/hacks/Labels/Label.cpp. Belum dikompilasi di sini.
- Workflow: `.github/workflows/build-all.yml` (Windows, macOS, Android32, Android64) menggantikan `android64.yml`.

## Perbaikan build Windows & macOS (11 Nov 2026)
- `src/modules/utils/GameCheckpoint.hpp`: bindings 2.2091 desktop tidak punya `PlayerObject::m_ringRelatedSet` dan
  `m_jumpPadRelated` (error di PracticeFix.cpp). Semua member khusus-desktop kini disalin lewat makro
  `ECLIPSE_SAVE_FIELD` / `ECLIPSE_LOAD_FIELD` yang melewati member yang tidak ada.
- `GameObjectPhysics` tidak bisa di-copy-assign di bindings ini (error `std::list ... _Assign_cast` di MSVC STL);
  `m_gameObjectPhysics` disalin lewat `copyMapSafely` (assign -> copy elemen -> dilewati).
- Efek: di desktop, state ring/jump pad pada checkpoint PracticeFix hanya dipulihkan kalau member-nya ada di bindings.

## Perbaikan crash load: ColorPickPopup::textChanged (14 Nov 2026)
- Error di HP: `dlopen failed: cannot locate symbol _ZN5geode14ColorPickPopup11textChangedEP15CCTextInputNode`.
  Penyebab: kelas `ColorPopup` (turunan `geode::ColorPickPopup`) membuat vtable yang merujuk `textChanged`, simbol itu
  tidak diekspor oleh loader Geode di perangkat.
- Perbaikan: `ColorPopup` dihapus; color picker langsung memakai `geode::ColorPickPopup::create(...)` (tanpa subclass).
- `CocosRenderer::m_extraPopups` kini `Ref<CCNode>` dan popup yang sudah ditutup dibersihkan saat `registerModal`,
  karena popup warna tidak lagi unregister sendiri lewat destruktor. Belum dikompilasi di sini.

## Workflow: iOS ditambahkan (14 Nov 2026)
- `build-all.yml` kini membuild Windows, macOS, iOS, Android32, Android64. Job iOS memakai `target: iOS` di `macos-latest`.
- Belum pernah dikompilasi untuk iOS di sini; kalau gagal, kirim lognya.

## Perbaikan build semua platform (14 Nov 2026, v12)
- `ColorPickPopup::create(color, isRGBA)` kini protected di Geode 6 (error di Android32/64, Windows, macOS, iOS).
  `color-picker.cpp` memakai struct pembuka akses `ColorPickPopupAccess` (turunan, tidak pernah diinstansiasi, jadi
  tidak ada vtable baru dan masalah simbol `textChanged` tidak kembali).
- iOS: header modify `CCKeyboardDispatcher` gagal ("expected expression") karena bindings tidak punya alamat iOS
  untuk ctor/dtor. `manager.cpp` tidak memakainya, include dilewati di iOS. Untuk `gd-imgui-cocos/hooks.cpp` (library
  luar) ditambah shim `src/Geode/modify/CCKeyboardDispatcher.hpp` yang mematikan hook ctor/dtor khusus iOS.
- Belum dikompilasi di sini. Kalau iOS masih gagal, kirim lognya.

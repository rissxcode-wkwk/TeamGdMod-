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

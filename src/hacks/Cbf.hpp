#pragma once
// Presisi klik: Klik di Langkah (Click on Steps) dan Click Between Frames (CBF).
// Kedua mode memakai stempel waktu input yang dicatat GD di m_queuedButtons.

namespace tgm::cbf {
    // Mode yang benar-benar berlaku saat ini (0 = bawaan GD). Otomatis 0 saat bot merekam / memutar,
    // di luar level, atau bila stempel waktu input terdeteksi tidak cocok.
    int modeEfektif();

    // Dipanggil di awal tiap frame (CCScheduler::update), mencatat waktu frame
    void awalFrame();

    // Dipanggil dari processCommands. Mode Klik di Langkah: memproses satu langkah input.
    void sebelumProcessCommands();

    // Status untuk ditampilkan di menu
    char const* status();

    // Dipanggil saat level dimuat: reset keadaan dan pasang flag GD sesuai mode
    void levelDimuat();
}

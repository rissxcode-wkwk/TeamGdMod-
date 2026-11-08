#pragma once
// Frame Stepper: membekukan level lalu maju satu langkah fisika setiap tombol Langkah ditekan.

namespace tgm::stepper {
    bool beku();            // level sedang dibekukan
    void ubahBeku();        // beku / lanjut (musik ikut dijeda / dilanjutkan)
    void langkah();         // minta satu langkah fisika
    bool ambilLangkah();    // dipakai hook update: mengambil satu permintaan langkah
    void reset();           // dipanggil saat level dimuat atau ditutup
}

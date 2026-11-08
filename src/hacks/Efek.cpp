// Tanpa Partikel / Mode Ringan: partikel di dalam level tidak digambar.
// File ini berdiri sendiri: kalau build gagal gara-gara file ini, hapus saja (menu tetap jalan).

#include <Geode/Geode.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>

#include "../core/Config.hpp"

using namespace geode::prelude;

class $modify(TgmParticle, CCParticleSystemQuad) {
    void draw() {
        auto const& c = tgm::cfg();
        if ((c.tanpaPartikel || c.modeRingan) && PlayLayer::get()) return;
        CCParticleSystemQuad::draw();
    }
};

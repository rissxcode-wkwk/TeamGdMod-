// Hack tampilan: Tanpa Guncangan dan Tanpa Portal Cermin.

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include "../core/Config.hpp"

using namespace geode::prelude;

class $modify(TgmVisualLayer, GJBaseGameLayer) {
    void shakeCamera(float durasi, float kekuatan, float interval) {
        if (tgm::cfg().tanpaGuncang) return;
        GJBaseGameLayer::shakeCamera(durasi, kekuatan, interval);
    }

    void toggleFlipped(bool balik, bool tanpaEfek) {
        if (tgm::cfg().tanpaCermin) return;
        GJBaseGameLayer::toggleFlipped(balik, tanpaEfek);
    }
};

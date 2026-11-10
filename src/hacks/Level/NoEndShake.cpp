#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/toggle.hpp>
#include <modules/hack/hack.hpp>

#include <Geode/modify/PlayLayer.hpp>

namespace eclipse::hacks::Level {
    // GJGameState::m_unkPoint34 tidak ada di bindings GD 2.2091, jadi disimpan/dipulihkan hanya kalau field-nya ada.
    template <typename State>
    static auto saveUnkPoint34(State& state) {
        if constexpr (requires { state.m_unkPoint34; }) {
            auto value = state.m_unkPoint34;
            return [&state, value] { state.m_unkPoint34 = value; };
        } else {
            return [] {};
        }
    }

    class $hack(NoEndShake) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.level");
            tab->addToggle("level.noendshake")->setDescription()->handleKeybinds();
        }

        [[nodiscard]] const char* getId() const override { return "No End Shake"; }
    };

    REGISTER_HACK(NoEndShake)

    class $modify(NoEndShakePLHook, PlayLayer) {
        ADD_HOOKS_DELEGATE("level.noendshake")

        void showCompleteEffect() {
          auto unkUint64_1 = m_gameState.m_unkUint64_1;
          auto cameraShakeFactor = m_gameState.m_cameraShakeFactor;
          auto restoreUnkPoint34 = saveUnkPoint34(m_gameState);
          auto unkUint16 = m_gameState.m_unkUint16;
          auto unkUint15 = m_gameState.m_unkUint15;
          PlayLayer::showCompleteEffect();
          m_gameState.m_unkUint64_1 = unkUint64_1;
          m_gameState.m_cameraShakeFactor = cameraShakeFactor;
          restoreUnkPoint34();
          m_gameState.m_unkUint16 = unkUint16;
          m_gameState.m_unkUint15 = unkUint15;
        }
    };
}

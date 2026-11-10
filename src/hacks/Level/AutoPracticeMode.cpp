#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/toggle.hpp>
#include <modules/hack/hack.hpp>

#include <Geode/modify/PlayLayer.hpp>

namespace eclipse::hacks::Level {
    class $hack(AutoPracticeMode) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.level");
            tab->addToggle("level.autopracticemode")->handleKeybinds()->setDescription();
        }

        [[nodiscard]] const char* getId() const override { return "Auto Practice Mode"; }
    };

    REGISTER_HACK(AutoPracticeMode)

    class $modify(AutoPracticeModePLHook, PlayLayer) {
        ADD_HOOKS_DELEGATE("level.autopracticemode")

        bool init(GJGameLevel* level, bool dontCreateObjects) {
            if (!PlayLayer::init(level, dontCreateObjects))
                return false;

            this->togglePracticeMode(true);
            return true;
        }
    };
}

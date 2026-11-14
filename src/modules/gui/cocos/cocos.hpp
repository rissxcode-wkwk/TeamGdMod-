#pragma once
#include <Geode/utils/cocos.hpp>
#include <modules/gui/gui.hpp>

namespace eclipse::gui::cocos {

    class OptionsPopup;
    class ModalPopup;

    class Popup;
    class TabMenu;
    class TabButton;
    class ScrollLayer;
    class ContentView;

    class CocosRenderer final : public Renderer {
    public:
        static CocosRenderer* get() {
            auto& engine = Engine::get();
            if (engine.getRendererType() != RendererType::Cocos2d) return nullptr;
            return static_cast<CocosRenderer*>(engine.getRenderer());
        }

        void init() override;
        void toggle() override;
        [[nodiscard]] bool isToggled() const override;
        void shutdown() override;
        void shutdown(bool noCleanup);
        [[nodiscard]] RendererType getType() const override { return RendererType::Cocos2d; }

        [[nodiscard]] Popup* getPopup() const { return m_popup; }
        bool isPartOfPopup(cocos2d::CCNode* node) const;
        void queueAfterDrawing(Function<void()>&& func) override;
        void showPopup(eclipse::Popup&& popup) override;

        /// @brief Used to refresh the selected page contents in the popup. Use this when you edit components within the page.
        void refreshPage() const;

        /// @brief Get the selected tab in the popup. If popup is not open, returns an empty string.
        [[nodiscard]] std::string_view getSelectedTab() const;

        /// @brief Will queue the node to be closed after the main popup closes.
        void registerModal(cocos2d::CCNode* modal) {
            if (!m_popup) return;
            // buang popup yang sudah ditutup (tidak punya parent lagi)
            std::erase_if(m_extraPopups, [](auto const& n) { return !n || !n->getParent(); });
            m_extraPopups.emplace_back(modal);
        }
        /// @brief Will remove the node from the queue to be closed after the main popup closes.
        void unregisterModal(cocos2d::CCNode* modal) {
            std::erase_if(m_extraPopups, [modal](auto const& n) { return n.data() == modal; });
        }

    private:
        Popup* m_popup = nullptr;
        std::vector<geode::Ref<cocos2d::CCNode>> m_extraPopups;
    };

}
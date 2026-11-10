#pragma once
#include <filesystem>
#include <functional>
#include <vector>
#include "base-component.hpp"

namespace eclipse::gui {
    /// @brief Combo component for selecting entries from a path.
    class FilesystemComboComponent : public Component {
    public:
        explicit FilesystemComboComponent(std::string id, std::string title, asp::fs::Path directory);

        void onUpdate() override;

        /// @brief Set a callback function to be called when the component value changes.
        FilesystemComboComponent* callback(Function<void(int)>&& func);

        /// @brief Get the combo value.
        [[nodiscard]] asp::fs::PathView getValue() const;

        /// @brief Get the combo items.
        [[nodiscard]] std::vector<asp::fs::Path> const& getItems() const;

        /// @brief Set the combo value.
        void setValue(asp::fs::Path path) const;

        /// @brief Set the combo value.
        void setValue(int index) const;

        [[nodiscard]] std::string const& getId() const override;

        [[nodiscard]] std::string const& getTitle() const override;

        [[nodiscard]] std::string* getSearchBuffer();

        FilesystemComboComponent* setDescription(std::string description) override;

        void triggerCallback(int value);

    private:
        void globFiles();

    private:
        std::string m_id;
        std::string m_title;
        asp::fs::Path m_directory;
        std::vector<asp::fs::Path> m_items;
        Function<void(int)> m_callback;

        std::string m_searchBuffer;
    };
}

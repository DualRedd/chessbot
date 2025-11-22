#pragma once

#include "TGUI/TGUI.hpp"
#include "TGUI/Backend/SFML-Graphics.hpp"

#include "../../core/types.hpp"
#include "../player_configuration.hpp"

#include <optional>
#include <unordered_map>
#include <string>
#include <vector>

/**
 * TGUI grow layout for dynamically generating option widgets for player configuration.
 */
class PlayerConfigView {
public:
    /**
     * @param gui TGUI object this view will be part of
     * @param color Which player (black or white) this configuration view is for
     */
    PlayerConfigView(tgui::GrowVerticalLayout::Ptr parent, PlayerColor color);

    /**
     * @return PlayerConfiguration representing the current configurations selected in in the GUI for this player.
     */
    PlayerConfiguration get_current_configuration();

private:
    /**
     * Apply the selected AI from the dropdown to the configuration view.
     * @param selected selected item from the dropdown
     */
    void _apply_item_selected(const tgui::String& selected);

    /**
     * Recalculate the internal panel size and reordered fields after adding/removing fields.
     */
    void _recalculate_internal_panel();

private:
    tgui::Panel::Ptr m_container;
    tgui::ComboBox::Ptr m_dropdown;

    // Pre-created per-AI field panels and inputs
    std::unordered_map<std::string, std::vector<tgui::Widget::Ptr>> m_ai_field_containers;
    std::unordered_map<std::string, std::vector<tgui::Widget::Ptr>> m_ai_field_inputs;
    std::unordered_map<std::string, std::vector<ConfigField>> m_ai_fields;
    std::vector<tgui::Widget::Ptr> m_all_field_containers;
};

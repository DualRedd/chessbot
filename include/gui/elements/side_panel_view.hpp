#pragma once

#include <functional>
#include "TGUI/TGUI.hpp"
#include "TGUI/Backend/SFML-Graphics.hpp"

#include "player_config_view.hpp"

/**
 * TODO
 */
class SidePanelView {
public:
    /**
     * TODO
     * @param gui TGUI object this view will be part of
     */
    SidePanelView(tgui::Gui& gui);

    /**
     * Set the position of the top left corner the panel.
     * @param position screen space position
     */
    void set_position(sf::Vector2f position);

    /**
     * Set the size of the panel.
     * @param size width and height
     */
    void set_size(sf::Vector2f size);

    /**
     * Set a callback to called when the flip board button is pressed.
     * @param callback the callback
     */
    void on_flip_board_pressed(std::function<void()> callback);

    /**
     * Set a callback to called when the undo button is pressed.
     * @param callback the callback
     */
    void on_undo_pressed(std::function<void()> callback);

    /**
     * Set a callback to called when the new game button is pressed.
     * @param callback the callback
     */
    void on_new_game_pressed(std::function<void()> callback);

    /**
     * Set a callback to be called when the AI speed configuration is changed.
     * @param callback the callback
     */
    void on_ai_speed_changed(std::function<void(double)> callback);

    /**
     * @param side Which player (black or white) to get the configuration for
     * @return PlayerConfiguration representing the current configurations selected in in the GUI for this player.
     */
    PlayerConfiguration get_player_configuration(Color side);

private:
    // Background panel
    tgui::Panel::Ptr m_panel;

    // Layout top
    tgui::Button::Ptr m_undo_button;
    tgui::Button::Ptr m_new_game_button;
    tgui::Button::Ptr m_flip_board_button;
    ConfigFieldView::Ptr m_ai_speed_field;

    // Layout middle
    tgui::ScrollablePanel::Ptr m_middle_scroll_panel;
    tgui::GrowVerticalLayout::Ptr m_middle_grow_panel;
    PlayerConfigView m_black_player_config;
    PlayerConfigView m_white_player_config;
};

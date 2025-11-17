#pragma once

#include <functional>

#include "TGUI/TGUI.hpp"
#include "TGUI/Backend/SFML-Graphics.hpp"

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
     * Set a callback to called when the undo button is pressed.
     * @param callback the callback
     */
    void on_undo_pressed(std::function<void()> callback);

    /**
     * Set a callback to called when the new game button is pressed.
     * @param callback the callback
     */
    void on_new_game_pressed(std::function<void()> callback);

private:
    tgui::Panel::Ptr m_panel;
    tgui::Button::Ptr m_undo_button;
    tgui::Button::Ptr m_new_game_button;
};

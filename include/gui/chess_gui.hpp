#pragma once

#include "SFML/Graphics.hpp"
#include "TGUI/TGUI.hpp"
#include "TGUI/Backend/SFML-Graphics.hpp"

#include "game_manager.hpp"
#include "elements/chess_view.hpp"
#include "elements/side_panel_view.hpp"

/**
 * GUI for configuring and playing chess interactively with bots.
 */
class ChessGUI {
public:
    /**
     * @param window_width window width in pixels
     * @param window_height window height in pixels
     */
    ChessGUI(int window_width = 640, int window_height = 640);
    ~ChessGUI();

    /**
     * Run the chess GUI. Blocking call.
     */
    void run();

private:
    /**
     * Handle window events.
     */
    void _handle_events();

    /**
     * Draw next frame.
     */
    void _draw();

    /**
     * Draw error popup over the window.
     */
    void _draw_error_popup(sf::RenderWindow& window);

    /**
     * Update window elements to match new window size.
     */
    void _on_window_resize();

    /**
     * Position and scale views to the current window.
     */
    void _update_element_transforms();

private:
    // Game manager
    GameManager m_game_manager;

    // Window and TGUI
    sf::RenderWindow m_window;
    tgui::Gui m_tgui;

    // Error popup
    bool m_error_popup_active = false;
    std::string m_error_message;
    sf::Font m_font;

    // Views
    SidePanelView m_side_panel;
    ChessView m_chess_view;
};

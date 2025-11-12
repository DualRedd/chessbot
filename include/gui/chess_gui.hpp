#pragma once

#include "../core/chess.hpp"
#include "elements/board_view.hpp"

#include <SFML/Graphics.hpp>

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

    /**
     * Run the chess GUI.
     */
    void run();

private:
    /**
     * Handle window events.
     */
    void _handleEvents();

    /**
     * Handle GUI user moves.
     */
    bool onUserMoveAttempt(const UCI& uci);

    /**
     * Draw next frame.
     */
    void _draw();

    /**
     * Update view to match resized window.
     */
    void _onWindowResize();

    /**
     * Update element transforms to current view.
     */
    void _updateElementTransforms();

private:
    sf::RenderWindow m_window;
    Chess m_game;

    // GUI elements
    BoardView m_board_view;
};

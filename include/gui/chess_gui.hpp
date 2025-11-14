#pragma once

#include "../core/chess.hpp"
#include "../core/ai_player.hpp"
#include "elements/board_view.hpp"

#include <thread>
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
    void _handle_events();

    /**
     * Handle getting and applying AI moves.
     */
    void _hande_ai_moves();

    /**
     * Handle GUI user moves.
     */
    bool _on_gui_move(const UCI& uci);

    /**
     * Draw next frame.
     */
    void _draw();

    /**
     * Update view to match resized window.
     */
    void _on_window_resize();

    /**
     * Update element transforms to current view.
     */
    void _update_element_transforms();

private:
    sf::RenderWindow m_window;

    // Game
    Chess m_game;
    std::optional<std::unique_ptr<AIPlayer>> m_white_ai;
    std::optional<std::unique_ptr<AIPlayer>> m_black_ai;
    std::optional<std::shared_ptr<AsyncMoveTask>> m_ai_move;

    // GUI elements
    BoardView m_board_view;
};

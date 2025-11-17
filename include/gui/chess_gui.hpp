#pragma once

#include <thread>

#include "SFML/Graphics.hpp"
#include "TGUI/TGUI.hpp"
#include "TGUI/Backend/SFML-Graphics.hpp"

#include "../core/chess.hpp"
#include "../core/ai_player.hpp"

#include "elements/board_view.hpp"
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
     * Handle getting AI moves.
     */
    void _handle_ai_moves();

    /**
     * Handle GUI user moves.
     */
    bool _on_gui_move(const UCI& uci);

    /**
     * Try applying a move on the board.
     * @return true if succesfull
     */
    bool _try_make_move(const UCI& move);

    /**
     * Try undoing a move on the board.
     * @return true if succesfull
     */
    bool _try_undo_move();

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
    // Game
    Chess m_game;

    // AI
    std::optional<std::unique_ptr<AIPlayer>> m_white_ai;
    std::optional<std::unique_ptr<AIPlayer>> m_black_ai;
    std::optional<std::shared_ptr<AsyncMoveCompute>> m_ai_move;
    std::vector<std::string> m_white_ai_unapplied_moves; // Special string "UNDO" for undo operations
    std::vector<std::string> m_black_ai_unapplied_moves; // Special string "UNDO" for undo operations

    // Window and TGUI
    sf::RenderWindow m_window;
    tgui::Gui m_tgui;

    // Views
    SidePanelView m_side_panel;
    BoardView m_board_view;
};

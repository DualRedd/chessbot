#pragma once

#include "../core/chess.hpp"

#include <SFML/Graphics.hpp>
#include <array>
#include <map>

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
     * Return board bounds in pixels.
     */
    sf::FloatRect _get_board_bounds() const;

    /**
     * Convert a screen position to file and rank coords.
     * @return The file and rank containing the coordinate, or no value if out of bounds
     * @note File 0 rank 0 = bottom left square of the board
     */
    std::optional<std::pair<int, int>> _screen_to_board_space(const sf::Vector2f& screen_pos) const;

    /**
     * Convert file and rank to a screen coordinate.
     * @return The coordinate of the top left corner of this square.
     * @note File 0 rank 0 = bottom left square of the board
     */
    sf::Vector2f _board_to_screen_space(int file, int rank) const;

    /**
     * Load piece svg files.
     * @param asset_path path to the folder containing the svgs
     */
    void _loadPieceTexturesFromSVG(const std::string& asset_path);

    /**
     * Handles window events.
     */
    void _handleEvents();

    /**
     * Render frame entry point.
     */
    void _render();

    /**
     * Render board.
     */
    void _drawBoard();

    /**
     * Render pieces
     */
    void _drawPieces();

    /**
     * @return The color this square should be rendered with.
     */
    sf::Color _getSquareColor(int file, int rank);

private:
    Chess m_game;
    sf::RenderWindow m_window;
    sf::Font m_font;

    // Pieces
    std::unordered_map<Piece, sf::Texture, Piece::Hash> m_piece_textures;

    // State
    std::optional<std::pair<int, int>> m_selected_square;
    std::optional<Move> m_current_user_move;

    // Dragging
    bool m_is_dragging{false};
    sf::Vector2i m_drag_start_square;
    sf::Vector2f m_drag_screen_position;

    // Colors
    const static inline sf::Color s_light_square_color = sf::Color(240, 217, 181);
    const static inline sf::Color s_dark_square_color = sf::Color(181, 136, 99);
    const static inline sf::Color s_highlight_color = sf::Color(255, 255, 0, 100);
};

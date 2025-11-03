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
     * Load assets used by the gui.
     */
    void _loadAssets();

    /**
     * @return Board bounds in pixels.
     */
    sf::FloatRect _get_board_bounds() const;

    /**
     * @return Tile size in pixels.
     */
    float _get_tile_size() const;

    /**
     * Convert a screen position to a board tile.
     * @param screen_pos The position in screen space.
     * @return The tile containing the coordinate, or no value if out of bounds.
     * @note File 0 rank 0 = bottom left tile of the board
     */
    std::optional<BoardTile> _screen_to_board_space(const sf::Vector2f& screen_pos) const;

    /**
     * Convert a board tile to a screen coordinate.
     * @param tile The tile to convert.
     * @return The coordinate of the center of this tile.
     * @note File 0 rank 0 = bottom left tile of the board
     */
    sf::Vector2f _board_to_screen_space(const BoardTile& tile) const;

    /**
     * Handles window events.
     */
    void _handleEvents();

    /**
     * Draw frame entry point.
     */
    void _render();

    /**
     * Draw board.
     */
    void _drawBoard();

    /**
     * Draw legal moves.
     * @param tile The tile which the moves are rendered for.
     */
    void _drawLegalMoves(const BoardTile& tile);

    /**
     * Draw a piece.
     * @param piece the piece to draw
     * @param position draw position in screen space
     */
    void _drawPiece(const Piece& piece, const sf::Vector2f& position);

    /**
     * @param tile A board tile.
     * @return The color this tile should be rendered with.
     */
    sf::Color _getSquareColor(const BoardTile& tile);

private:
    Chess m_game;
    sf::RenderWindow m_window;
    sf::Font m_font;

    // Textures
    std::unordered_map<Piece, sf::Texture, Piece::Hash> m_texture_pieces;
    sf::Texture m_texture_circle;
    sf::Texture m_texture_circle_hollow;

    // State
    std::optional<BoardTile> m_selected_square;
    std::optional<Move> m_current_user_move;

    // Dragging
    bool m_is_dragging{false};
    BoardTile m_drag_start_square;
    sf::Vector2f m_drag_screen_position;

    // Colors
    const static inline sf::Color s_light_square_color = sf::Color(240, 217, 181);
    const static inline sf::Color s_dark_square_color = sf::Color(181, 136, 99);
    const static inline sf::Color s_highlight_color = sf::Color(255, 255, 0, 100);
};

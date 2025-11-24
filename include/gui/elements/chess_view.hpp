#pragma once

#include <map>
#include <functional>
#include "SFML/Graphics.hpp"

#include "../game_manager.hpp"

/**
 * Interactive board view for playing chess.
 */
class ChessView {
public:
    /**
     * @param game_manager the game manager this view is associated with.
     */
    ChessView(GameManager& game_manager);

    /**
     * Set the position of the top left corner of the board.
     * @param position screen space position
     */
    void set_position(sf::Vector2f position);

    /**
     * Set the size of the board.
     * @param size width and height of the board
     */
    void set_size(float size);

    /**
     * Flip the board (rotate 180 degrees).
     */
    void flip_board();

    /**
     * Draw the board.
     * @param window target for drawing
     * @param is_human_turn conditional drawing for human moves
     * */
    void draw(sf::RenderWindow& window);

    /**
     * @param event the event to handle
     * */
    void handle_event(const sf::Event& event);

private:                                                             /** Events */
    void _on_mouse_left_down(sf::Vector2i screen_position);
    void _on_mouse_left_up(sf::Vector2i screen_position);
    void _on_mouse_moved(sf::Vector2i screen_position);
    void _on_piece_moved(Chess::Tile from, Chess::Tile to, PieceType promotion = PieceType::None);
    void _on_game_end(Chess::GameState state);

private:                                                             /** Drawing */
    /**
     * Draw tiles with possible highlighting.
     * @param window target for drawing
     */
    void _draw_board(sf::RenderWindow& window);

    /**
     * Draw a vertical select of pieces.
     * @param window target for drawing
     */
    void _draw_promotion_prompt(sf::RenderWindow& window);

    /**
     * @param window target for drawing
     * @param tile The tile which the moves are drawn for.
     */
    void _draw_legal_moves(sf::RenderWindow& window, Chess::Tile tile);

    /**
     * @param window target for drawing
     * @param piece the piece to draw
     * @param position draw position in screen space
     */
    void _draw_piece(sf::RenderWindow& window, Piece piece, sf::Vector2f position);

    /**
     * @param window target for drawing
     */
    void _draw_game_over_popup(sf::RenderWindow& window);

    /**
     * @param tile A board tile.
     * @return The color this tile should be rendered with.
     */
    sf::Color _get_tile_color(Chess::Tile tile);

private:                                                             /** Private helpers */
    /**
     * Load pieces and other UI assets.
     */
    void _load_assets();

    /**
     * Convert a screen position to a board tile.
     * @param screen_pos The position in screen space.
     * @return The tile containing the coordinate, or no value if out of bounds.
     * @note File 0 rank 0 = bottom left tile of the board
     */
    std::optional<Chess::Tile> _screen_to_board_space(sf::Vector2f screen_pos) const;

    /**
     * Convert a board tile to a screen coordinate.
     * @param tile The tile to convert.
     * @return The coordinate of the center of this tile.
     * @note File 0 rank 0 = bottom left tile of the board
     */
    sf::Vector2f _board_to_screen_space(Chess::Tile tile) const;

    /**
     * Convert a game state to a user friendly message.
     * @param state the game state
     * @return message describing the game state
     */
    std::string _game_state_to_message(Chess::GameState state) const;

private:
    GameManager& m_game_manager;

    // Transform
    bool m_flipped = false;
    sf::Vector2f m_position;
    float m_size;
    float m_tile_size;

    // Textures
    std::unordered_map<Piece, sf::Texture, Piece::Hash> m_texture_pieces;
    sf::Texture m_texture_circle;
    sf::Texture m_texture_circle_hollow;
    sf::Texture m_texture_x_icon;

    // Dragging
    bool m_is_dragging = false;
    sf::Vector2f m_drag_screen_position;
    std::optional<Chess::Tile> m_selected_tile;
    bool m_move_was_applied = false;

    // Promotion
    const static inline PieceType s_promotion_pieces[4] = {PieceType::Queen, PieceType::Rook, PieceType::Knight, PieceType::Bishop};
    bool m_promotion_prompt_active = false;
    Chess::Tile m_promotion_prompt_tile;

    // Game over status popup
    bool m_game_over_popup_active = false;
    std::string m_game_over_message;
    sf::Font m_font;

    // Colors
    const static inline sf::Color s_light_tile_color = sf::Color(240, 217, 181);
    const static inline sf::Color s_dark_tile_color = sf::Color(181, 136, 99);
    const static inline sf::Color s_highlight_color = sf::Color(255, 255, 0, 100);
};

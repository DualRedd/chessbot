#pragma once

#include <core/game.hpp>

#include <SFML/Graphics.hpp>
#include <array>
#include <map>

enum class SquareState { Normal, Selected };

class ChessGUI {
public:
    ChessGUI(Game& game, int windowWidth = 640, int windowHeight = 640);

     /**
     * Main render and input poll loop.
     */
    void run();

private:
    /**
     * Return board bounds in pixels.
     */
    sf::FloatRect _get_board_bounds() const;
    /**
     * Convert a screen position to file and rank coords.
     * @return the file and rank containing the coordinate, or no value if out of bounds
     */
    std::optional<std::pair<int, int>> _screen_to_board_space(const sf::Vector2f& screen_pos) const;
    /**
     * Convert file and rank to a screen coordinate.
     * @return the coordinate of the top left corner
     */
    sf::Vector2f _board_to_screen_space(int file, int rank) const;

    void _handleEvents();
    void _render();
    void _drawBoard();
    void _drawPieces();
    sf::Color _getSquareColor(int file, int rank);

    void _handleClick(sf::Vector2i pos);

private:
    static constexpr int BOARD_SIZE = 8;

    Game& m_game;
    sf::RenderWindow m_window;
    sf::Font m_font;

    // Pieces
    std::map<char, sf::Texture> m_piece_textures;
    std::map<char, sf::Sprite> m_piece_sprites;

    // State
    std::optional<std::pair<int, int>> m_selected_square;

    // Colors
    const static inline sf::Color s_light_square_color = sf::Color(240, 217, 181);
    const static inline sf::Color s_dark_square_color = sf::Color(181, 136, 99);
    const static inline sf::Color s_highlight_color = sf::Color(255, 255, 0, 100);
};

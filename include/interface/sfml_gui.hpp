#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <map>

// TODO: Define the game class
class Game;

class ChessGUI {
public:
    ChessGUI(Game& game, int windowWidth = 640, int windowHeight = 640);

     /**
     * Main render and input poll loop.
     */
    void run();

private:
    void _handleEvents();
    void _render();
    void _drawBoard();
    void _drawPieces();

    /**
     * Convert mouse position to board square (0–7, 0–7)
     */
    std::pair<int, int> _getSquareFromMousePosition(const sf::Vector2i& mousePos) const;

private:
    Game& m_game;

    sf::RenderWindow m_window;
    sf::Font m_font;

    // Board configuration
    static constexpr int BOARD_SIZE = 8;
    float m_tile_size;

    // Pieces
    std::map<char, sf::Texture> m_piece_textures;
    std::map<char, sf::Sprite> m_piece_sprites;

    // Colors
    sf::Color m_light_square_color = sf::Color(240, 217, 181);
    sf::Color m_dark_square_color  = sf::Color(181, 136, 99);
};

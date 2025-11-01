#include "interface/sfml_gui.hpp"

#include <algorithm>

ChessGUI::ChessGUI(Game& game, int windowWidth, int windowHeight)
    : m_game(game),
      m_window(sf::VideoMode(sf::Vector2u(windowWidth, windowHeight)), "Chess"),
      m_tile_size(static_cast<float>(windowWidth) / BOARD_SIZE)
{
    m_window.setFramerateLimit(60);

    // TODO: load piece textures from assets
}

void ChessGUI::run() {
    while (m_window.isOpen()) {
        _handleEvents();
        _render();
    }
}

void ChessGUI::_handleEvents() {
    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()){
            m_window.close();
        }
        else if (const auto* mouse_event = event->getIf<sf::Event::MouseButtonPressed>()) {
            auto [file, rank] = _getSquareFromMousePosition(mouse_event->position);

            // Handle click here
        } 
    }
}

void ChessGUI::_render() {
    m_window.clear();
    _drawBoard();
    _drawPieces();
    m_window.display();
}

void ChessGUI::_drawBoard() {
    sf::RectangleShape square(sf::Vector2f(m_tile_size, m_tile_size));

    for (int rank = 0; rank < BOARD_SIZE; ++rank) {
        for (int file = 0; file < BOARD_SIZE; ++file) {
            bool isLight = (rank + file) % 2 == 0;
            square.setFillColor(isLight ? m_light_square_color : m_dark_square_color);
            square.setPosition(sf::Vector2f(file * m_tile_size, rank * m_tile_size));
            m_window.draw(square);
        }
    }
}

void ChessGUI::_drawPieces() {
    // TODO
}

std::pair<int, int> ChessGUI::_getSquareFromMousePosition(const sf::Vector2i& mousePos) const {
    int file = mousePos.x / m_tile_size;
    int rank = mousePos.y / m_tile_size;
    file = std::clamp(file, 0, BOARD_SIZE - 1);
    rank = std::clamp(rank, 0, BOARD_SIZE - 1);
    return { file, rank };
}
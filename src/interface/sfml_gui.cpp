#include "interface/sfml_gui.hpp"

#include <algorithm>

ChessGUI::ChessGUI(Game& game, int windowWidth, int windowHeight)
    : m_game(game),
      m_window(sf::VideoMode(sf::Vector2u(windowWidth, windowHeight)), "Chess")
{
    m_window.setFramerateLimit(60);

    // TODO: load piece textures from assets
}

sf::FloatRect ChessGUI::_get_board_bounds() const {
    sf::Vector2u window_size = m_window.getSize();
    float board_size_pixels = std::min(window_size.x, window_size.y);
    float offsetX = (window_size.x - board_size_pixels) / 2.f;
    float offsetY = (window_size.y - board_size_pixels) / 2.f;
    return sf::FloatRect(sf::Vector2f(offsetX, offsetY), sf::Vector2f(board_size_pixels, board_size_pixels));
}

std::optional<std::pair<int, int>> ChessGUI::_screen_to_board_space(const sf::Vector2f& screen_pos) const {
    sf::FloatRect board_rect = _get_board_bounds();
    if (!board_rect.contains(screen_pos)) {
        return std::nullopt;
    }
    float tile_size = board_rect.size.x / BOARD_SIZE;
    int file = static_cast<int>((screen_pos.x - board_rect.position.x) / tile_size);
    int rank = static_cast<int>((screen_pos.y - board_rect.position.y) / tile_size);
    // safety check in case of rounding
    if (file < 0 || file >= BOARD_SIZE || rank < 0 || rank >= BOARD_SIZE){
        return std::nullopt;
    }
    return std::make_pair(file, rank);
}

sf::Vector2f ChessGUI::_board_to_screen_space(int file, int rank) const {
    sf::FloatRect board_rect = _get_board_bounds();
    float tile_size = board_rect.size.x / BOARD_SIZE;
    return sf::Vector2f(board_rect.position.x + file * tile_size, board_rect.position.y + rank * tile_size);
}


void ChessGUI::run() {
    while (m_window.isOpen()) {
        _handleEvents();
        _render();
    }
}

void ChessGUI::_handleEvents() {
    bool resized = false;

    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()){
            m_window.close();
        }
        else if (const auto& resize_event = event->getIf<sf::Event::Resized>()) {
            resized = true;
        }
        else if (const auto* mouse_event = event->getIf<sf::Event::MouseButtonPressed>()) {
            _handleClick(mouse_event->position);
        } 
    }

    // Separate check so multiple resize events only trigger one view change each frame
    if(resized){
        sf::Vector2u size = m_window.getSize(); 
        sf::FloatRect rect(sf::Vector2f(0.0, 0.0), sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
        m_window.setView(sf::View(rect));
    }
}

void ChessGUI::_render() {
    m_window.clear();
    _drawBoard();
    _drawPieces();
    m_window.display();
}

void ChessGUI::_drawBoard() {
    float tile_size = _get_board_bounds().size.x / BOARD_SIZE;
    sf::RectangleShape square(sf::Vector2f(tile_size, tile_size));
    for (int rank = 0; rank < BOARD_SIZE; rank++) {
        for (int file = 0; file < BOARD_SIZE; file++) {
            square.setFillColor(_getSquareColor(file, rank));
            square.setPosition(_board_to_screen_space(file, rank));
            m_window.draw(square);
        }
    }
}

void ChessGUI::_drawPieces() {
    // TODO
}

sf::Color ChessGUI::_getSquareColor(int file, int rank) {
    sf::Color color = (rank + file) % 2 == 0 ? s_light_square_color : s_dark_square_color;
    if (m_selected_square && m_selected_square->first == file && m_selected_square->second == rank){
        // alpha blending
        color.r = (color.r * (255 - s_highlight_color.a) + s_highlight_color.r * s_highlight_color.a) / 255;
        color.g = (color.g * (255 - s_highlight_color.a) + s_highlight_color.g * s_highlight_color.a) / 255;
        color.b = (color.b * (255 - s_highlight_color.a) + s_highlight_color.b * s_highlight_color.a) / 255;
    }
    return color;
}

void ChessGUI::_handleClick(sf::Vector2i pos){
    if(const auto board_pos = _screen_to_board_space(sf::Vector2f(pos.x, pos.y))){
        auto[file, rank] = board_pos.value();
        m_selected_square = std::make_pair(file, rank);
    }
}

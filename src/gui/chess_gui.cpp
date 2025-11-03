#include "gui/chess_gui.hpp"
#include "core/assets.hpp"

// NanoSVG
#include <stdio.h>
#include <string.h>
#include <math.h>
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "extern/nanosvgrast.h"


ChessGUI::ChessGUI(int window_width, int window_height)
    : m_game(),
      m_window(sf::VideoMode(sf::Vector2u(window_width, window_height)), "Chess")
{
    // Center window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    int posX = (int(desktop.size.x) - int(window_width)) / 2;
    int posY = (int(desktop.size.y) - int(window_height)) / 2;
    m_window.setPosition(sf::Vector2i(posX, posY));

    m_window.setFramerateLimit(60);
    _loadPieceTexturesFromSVG(getExecutableDir() / "assets" / "pieces");
}

void ChessGUI::run() {
    m_game.new_board();

    while (m_window.isOpen()) {
        _handleEvents();

        if(m_current_user_move.has_value()){
            m_game.play_move(m_current_user_move.value());
            m_selected_square.reset();
        }

        _render();
    }
}

sf::FloatRect ChessGUI::_get_board_bounds() const {
    sf::Vector2u window_size = m_window.getSize();
    float board_size_pixels = std::min(window_size.x, window_size.y);
    float offsetX = (window_size.x - board_size_pixels) / 2.f;
    float offsetY = (window_size.y - board_size_pixels) / 2.f;
    return sf::FloatRect(sf::Vector2f(offsetX, offsetY), sf::Vector2f(board_size_pixels, board_size_pixels));
}

std::optional<BoardTile> ChessGUI::_screen_to_board_space(const sf::Vector2f& screen_pos) const {
    sf::FloatRect board_rect = _get_board_bounds();
    if (!board_rect.contains(screen_pos)) {
        return std::nullopt;
    }
    float tile_size = board_rect.size.x / CHESSBOARD_SIZE;
    int file = static_cast<int>((screen_pos.x - board_rect.position.x) / tile_size);    
    int rank = CHESSBOARD_SIZE-1-static_cast<int>((screen_pos.y - board_rect.position.y) / tile_size);
    // safety check in case of rounding
    if (file < 0 || file >= CHESSBOARD_SIZE || rank < 0 || rank >= CHESSBOARD_SIZE){
        return std::nullopt;
    }
    return BoardTile(file, rank);
}

sf::Vector2f ChessGUI::_board_to_screen_space(const BoardTile& tile) const {
    sf::FloatRect board_rect = _get_board_bounds();
    float tile_size = board_rect.size.x / CHESSBOARD_SIZE;
    return sf::Vector2f(board_rect.position.x + tile.file * tile_size, board_rect.position.y + (CHESSBOARD_SIZE-1-tile.rank) * tile_size);
}

void ChessGUI::_loadPieceTexturesFromSVG(const std::string& folder_path) {
    NSVGrasterizer* rast = nsvgCreateRasterizer();

    auto loadSVG = [&](PieceType type, PlayerColor color, const std::string& filename) {
        std::string path = folder_path + "/" + filename;

        // Parse SVG
        NSVGimage* image = nsvgParseFromFile(path.c_str(), "px", 96.0f);
        if (!image) {
            throw std::runtime_error("Failed to parse SVG: " + path);
        }

        // Rasterize to a buffer
        constexpr int render_size = 128;
        std::vector<unsigned char> bitmap(render_size * render_size * 4); // RGBA
        nsvgRasterize(rast, image, 0, 0, (float)render_size / image->width,
                      bitmap.data(), render_size, render_size, render_size * 4);
        nsvgDelete(image);

        // Create texture
        sf::Image img(sf::Vector2u(render_size, render_size), bitmap.data());
        sf::Texture tex;
        if(!tex.loadFromImage(img)){
            throw std::runtime_error("Failed to create image from rasterized svg: " + path);
        }
        tex.setSmooth(true);

        m_piece_textures[{type, color}] = tex;
    };

    for(PlayerColor color : {PlayerColor::White, PlayerColor::Black}){
        std::string prefix = color == PlayerColor::White ? "w" : "b";
        loadSVG(PieceType::Pawn,   color, prefix + "P.svg");
        loadSVG(PieceType::Knight, color, prefix + "N.svg");
        loadSVG(PieceType::Bishop, color, prefix + "B.svg");
        loadSVG(PieceType::Rook,   color, prefix + "R.svg");
        loadSVG(PieceType::Queen,  color, prefix + "Q.svg");
        loadSVG(PieceType::King,   color, prefix + "K.svg");
    }

    nsvgDeleteRasterizer(rast);
}

void ChessGUI::_handleEvents() {
    bool resized = false;
    m_current_user_move.reset();

    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()){
            m_window.close();
        }

        else if (const auto& resize_event = event->getIf<sf::Event::Resized>()) {
            resized = true;
        }

        else if (const auto& mouse_press_event = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mouse_press_event->button == sf::Mouse::Button::Left) {
                if(const auto tile = _screen_to_board_space(sf::Vector2f(mouse_press_event->position))){
                    if (m_game.get_board().get_piece(tile.value()).type != PieceType::None) {
                        m_is_dragging = true;
                        m_drag_start_square = tile.value();
                        m_selected_square = tile.value();
                        m_drag_screen_position = sf::Vector2f(mouse_press_event->position);
                    }
                    else{
                        m_selected_square.reset();
                    }
                }
            }
        }

        else if (const auto& mouse_released_event = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (m_is_dragging && mouse_released_event->button == sf::Mouse::Button::Left) {
                m_is_dragging = false;
                if(const auto tile = _screen_to_board_space(sf::Vector2f(mouse_released_event->position))){
                    if(tile != m_drag_start_square){
                        m_current_user_move = Move(m_drag_start_square, tile.value());
                    }
                }
            }
        }

        else if (const auto& mouse_move_event = event->getIf<sf::Event::MouseMoved>()) {
            if (m_is_dragging) {
                m_drag_screen_position = sf::Vector2f(mouse_move_event->position);
            }
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
    float tile_size = _get_board_bounds().size.x / CHESSBOARD_SIZE;
    sf::RectangleShape square(sf::Vector2f(tile_size, tile_size));
    for (int rank = 0; rank < CHESSBOARD_SIZE; rank++) {
        for (int file = 0; file < CHESSBOARD_SIZE; file++) {
            BoardTile tile(file, rank);
            square.setFillColor(_getSquareColor(tile));
            square.setPosition(_board_to_screen_space(tile));
            m_window.draw(square);
        }
    }
}

void ChessGUI::_drawLegalMoves(const BoardTile& tile) {
    auto moves = m_game.get_board().get_legal_moves(tile);
    // TODO:
}

void ChessGUI::_drawPieces() {
    float tile_size = _get_board_bounds().size.x / CHESSBOARD_SIZE;

    for (int rank = 0; rank < CHESSBOARD_SIZE; rank++) {
        for (int file = 0; file < CHESSBOARD_SIZE; file++) {
            BoardTile tile(file, rank);
            const Piece& p = m_game.get_board().get_piece(tile);
            if (p.type == PieceType::None) continue;

            sf::Sprite sprite(m_piece_textures[{p.type, p.color}]);

            if(m_is_dragging && tile == m_drag_start_square){
                sprite.setPosition(m_drag_screen_position - sf::Vector2f(tile_size / 2.f, tile_size / 2.f));
            }
            else{
                sprite.setPosition(_board_to_screen_space(tile));
            }
            
            float scale = tile_size / sprite.getTexture().getSize().x;
            sprite.setScale(sf::Vector2f(scale, scale));
            m_window.draw(sprite);
        }
    }
}


sf::Color ChessGUI::_getSquareColor(const BoardTile& tile) {
    sf::Color color = (tile.rank + tile.file) % 2 == 0 ? s_light_square_color : s_dark_square_color;
    auto last_move = m_game.get_last_move();
    if ((m_selected_square.has_value() && m_selected_square == tile)
        || (last_move.has_value() && (last_move.value().from == tile || last_move.value().to == tile)))
    {
        // alpha blending highlight
        color.r = (color.r * (255 - s_highlight_color.a) + s_highlight_color.r * s_highlight_color.a) / 255;
        color.g = (color.g * (255 - s_highlight_color.a) + s_highlight_color.g * s_highlight_color.a) / 255;
        color.b = (color.b * (255 - s_highlight_color.a) + s_highlight_color.b * s_highlight_color.a) / 255;
    }
    return color;
}

#include "gui/chess_gui.hpp"
#include "gui/assets.hpp"
#include<iostream>
ChessGUI::ChessGUI(int window_width, int window_height)
    : m_game(),
      m_window(sf::VideoMode(sf::Vector2u(window_width, window_height)), "Chess")
{
    // Center window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    int pos_x = (static_cast<int>(desktop.size.x) - window_width) / 2;
    int pos_y = (static_cast<int>(desktop.size.y) - window_height) / 2;
    m_window.setPosition(sf::Vector2i(pos_x, pos_y));

    m_window.setFramerateLimit(60);
    _loadAssets();
}

void ChessGUI::_loadAssets() {
    // Pieces
    for(PlayerColor color : {PlayerColor::White, PlayerColor::Black}){
        std::string prefix = (color == PlayerColor::White ? "w" : "b");
        m_texture_pieces[{PieceType::Pawn,   color}] = loadSVG("pieces/" + prefix + "P.svg");
        m_texture_pieces[{PieceType::Knight, color}] = loadSVG("pieces/" + prefix + "N.svg");
        m_texture_pieces[{PieceType::Bishop, color}] = loadSVG("pieces/" + prefix + "B.svg");
        m_texture_pieces[{PieceType::Rook,   color}] = loadSVG("pieces/" + prefix + "R.svg");
        m_texture_pieces[{PieceType::Queen,  color}] = loadSVG("pieces/" + prefix + "Q.svg");
        m_texture_pieces[{PieceType::King,   color}] = loadSVG("pieces/" + prefix + "K.svg");
    }

    // Other
    m_texture_circle = loadSVG("circle.svg");
    m_texture_circle_hollow = loadSVG("circle_hollow.svg");
    m_texture_x_icon = loadSVG("x_icon.svg");
}

void ChessGUI::run() {
    m_game.new_board();

    while (m_window.isOpen()) {
        _handleEvents();

        if(m_current_user_move.has_value()){
            bool played = m_game.play_move(m_current_user_move.value());
            if(played) m_selected_tile.reset();
        }

        _draw();
    }
}

sf::FloatRect ChessGUI::_get_board_bounds() const {
    sf::Vector2u window_size = m_window.getSize();
    float board_size_pixels = std::min(window_size.x, window_size.y);
    float offsetX = (window_size.x - board_size_pixels) / 2.f;
    float offsetY = (window_size.y - board_size_pixels) / 2.f;
    return sf::FloatRect(sf::Vector2f(offsetX, offsetY), sf::Vector2f(board_size_pixels, board_size_pixels));
}

float ChessGUI::_get_tile_size() const {
    return _get_board_bounds().size.x / 8;
}

std::optional<Chess::Tile> ChessGUI::_screen_to_board_space(const sf::Vector2f& screen_pos) const {
    sf::FloatRect board_rect = _get_board_bounds();
    if (!board_rect.contains(screen_pos)) {
        return std::nullopt;
    }
    float tile_size = _get_tile_size();
    int file = static_cast<int>((screen_pos.x - board_rect.position.x) / tile_size);    
    int rank = 7-static_cast<int>((screen_pos.y - board_rect.position.y) / tile_size);
    // safety check in case of rounding
    if (file < 0 || file >= 8 || rank < 0 || rank >= 8){
        return std::nullopt;
    }
    return Chess::Tile(file, rank);
}

sf::Vector2f ChessGUI::_board_to_screen_space(const Chess::Tile& tile) const {
    sf::FloatRect board_rect = _get_board_bounds();
    float tile_size = _get_tile_size();
    return sf::Vector2f(board_rect.position.x + tile.file *  tile_size + 0.5f * tile_size,
                        board_rect.position.y + (7-tile.rank) * tile_size + 0.5f * tile_size);
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
                _onMouseLeftDown(mouse_press_event->position);
            }
        }
        else if (const auto& mouse_released_event = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (mouse_released_event->button == sf::Mouse::Button::Left) {
                _onMouseLeftUp(mouse_released_event->position);
            }
        }
        else if (const auto& mouse_move_event = event->getIf<sf::Event::MouseMoved>()) {
            _onMouseMoved(mouse_move_event->position);
        }
    }

    // Separate check so multiple resize events only trigger one view change each frame
    if(resized){
        sf::Vector2u size = m_window.getSize(); 
        sf::FloatRect rect(sf::Vector2f(0.0, 0.0), sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
        m_window.setView(sf::View(rect));
    }
}

void ChessGUI::_onMouseLeftDown(const sf::Vector2i& screen_position) {
    if(const auto tile = _screen_to_board_space(sf::Vector2f(screen_position))){
        if(m_promotion_prompt_active) {
            // Check if user pressed an option or not
            if(tile.value().file == m_promotion_prompt_tile.file && std::abs(tile.value().rank - m_promotion_prompt_tile.rank) <= 3) {
                // Make move with the promotion piece chosen from the list
                m_current_user_move = Chess::uci_create(m_selected_tile.value(), m_promotion_prompt_tile,
                                    s_promotion_pieces[std::abs(tile.value().rank - m_promotion_prompt_tile.rank)]);
            }
            // In both cases the prompt should be closed
            m_promotion_prompt_active = false;
            m_selected_tile.reset();
        }
        else{
            // If there is a previous selection, make a move between these tiles
            if(m_selected_tile.has_value()){
                _onPieceMoved(m_selected_tile.value(), tile.value());
            }

            // Start a drag if the tile has some piece
            if (m_game.get_piece_at(tile.value()).type != PieceType::None) {
                m_is_dragging = true;
                m_selected_tile = tile;
                m_drag_screen_position = sf::Vector2f(screen_position);
            }
            else { // else empty tile was pressed, so reset selection
                m_selected_tile.reset();
            }
        }
    }
}

void ChessGUI::_onMouseLeftUp(const sf::Vector2i& screen_position){
    if(m_is_dragging){
        m_is_dragging = false;
        if(const auto tile = _screen_to_board_space(sf::Vector2f(screen_position))){
            // If there is a previous selection, make a move between these tiles
            if(m_selected_tile.has_value()){
                _onPieceMoved(m_selected_tile.value(), tile.value());
            }
        }
    }
}

void ChessGUI::_onMouseMoved(const sf::Vector2i& screen_position) {
    if (m_is_dragging) {
        m_drag_screen_position = sf::Vector2f(screen_position);
    }
}

void ChessGUI::_onPieceMoved(const Chess::Tile& from, const Chess::Tile& to) {
    bool is_promotion_move = m_game.is_legal_move(Chess::uci_create(from, to, s_promotion_pieces[0]));
    if(is_promotion_move) {
        m_promotion_prompt_active = true;
        m_promotion_prompt_tile = to;
    }
    else{
        m_current_user_move = Chess::uci_create(from, to);
    }
}

void ChessGUI::_draw() {
    m_window.clear();
    _drawBoard();

    // Stationary pieces
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            Chess::Tile tile(file, rank);
            if((m_is_dragging || m_promotion_prompt_active) && tile == m_selected_tile) continue;
            _drawPiece(m_game.get_piece_at(tile), _board_to_screen_space(tile));
        }
    }
    
    // Legal move highlights
    if(m_selected_tile.has_value()){
        _drawLegalMoves(m_selected_tile.value());
    }

    // Promotion prompt
    if(m_promotion_prompt_active){
        _drawPromotionPrompt();
    }

    // Dragged piece
    if(m_is_dragging && m_selected_tile.has_value()){
        _drawPiece(m_game.get_piece_at(m_selected_tile.value()), m_drag_screen_position);
    }

    m_window.display();
}

void ChessGUI::_drawBoard() {
    float tile_size = _get_tile_size();
    sf::RectangleShape square(sf::Vector2f(tile_size, tile_size));
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            Chess::Tile tile(file, rank);
            square.setOrigin(sf::Vector2f(square.getSize()) / 2.0f);
            square.setPosition(_board_to_screen_space(tile));
            square.setFillColor(_getTileColor(tile));
            m_window.draw(square);
        }
    }
}

void ChessGUI::_drawPromotionPrompt() {
    const float tile_size = _get_tile_size();
    int dir = m_promotion_prompt_tile.rank == 7 ? -1 : 1;
    PlayerColor color = m_promotion_prompt_tile.rank == 7 ? PlayerColor::White : PlayerColor::Black;

    // Draw background
    sf::RectangleShape background;
    const float background_height = 4.5f;
    background.setFillColor(sf::Color(233, 233, 233, 255));
    background.setSize(sf::Vector2f(tile_size, tile_size * background_height));
    background.setOrigin(sf::Vector2f(background.getSize()) / 2.0f);
    background.setPosition(_board_to_screen_space(m_promotion_prompt_tile) - sf::Vector2f(0.f, tile_size * (background_height-1.f) / 2.f * dir));
    m_window.draw(background);

    // Draw pieces
    for (int i = 0; i < 4; i++) {
        Chess::Tile tile(m_promotion_prompt_tile.file, m_promotion_prompt_tile.rank + i * dir);
        _drawPiece(Piece(s_promotion_pieces[i], color), _board_to_screen_space(tile));
    }

    // Draw x icon
    sf::Sprite sprite(m_texture_x_icon);
    float scale = tile_size / sprite.getTexture().getSize().x * 0.2f; 
    sprite.setOrigin(sf::Vector2f(sprite.getTexture().getSize()) / 2.0f);
    sprite.setPosition(_board_to_screen_space(m_promotion_prompt_tile) - sf::Vector2f(0.f, tile_size * (background_height - 0.75f) * dir));
    sprite.setScale(sf::Vector2f(scale, scale));
    sprite.setColor(sf::Color(255, 255, 255, 195));
    m_window.draw(sprite);
}

void ChessGUI::_drawLegalMoves(const Chess::Tile& tile) {
    std::vector<UCI> moves(m_game.get_legal_moves());
    if (moves.empty()) return;

    float tile_size = _get_tile_size();
    for (const UCI& move : moves) {
        auto[from, to, promo] = Chess::uci_parse(move);
        if(from != tile) continue;

        if(promo != PieceType::None && promo != PieceType::Queen){
            continue; // avoid duplicating sprites on promotion moves
        }

        bool is_capture = (m_game.get_piece_at(to).type != PieceType::None);

        sf::Sprite sprite(is_capture ? m_texture_circle_hollow : m_texture_circle);
        float scale = tile_size / sprite.getTexture().getSize().x * (is_capture ? 0.95f : 0.3f); 

        sprite.setOrigin(sf::Vector2f(sprite.getTexture().getSize()) / 2.0f);
        sprite.setPosition(_board_to_screen_space(to));
        sprite.setScale(sf::Vector2f(scale, scale));
        sprite.setColor(sf::Color(255, 255, 255, 70));
        m_window.draw(sprite);
    }
}

void ChessGUI::_drawPiece(const Piece& piece, const sf::Vector2f& position) {
    if (piece.type == PieceType::None) return;

    sf::Sprite sprite(m_texture_pieces[piece]);

    sprite.setOrigin(sf::Vector2f(sprite.getTexture().getSize()) / 2.0f);
    sprite.setPosition(position);
    
    float scale = _get_tile_size() / sprite.getTexture().getSize().x;
    sprite.setScale(sf::Vector2f(scale, scale));

    m_window.draw(sprite);
}

sf::Color ChessGUI::_getTileColor(const Chess::Tile& tile) {
    sf::Color color = (tile.rank + tile.file) % 2 == 0 ? s_light_tile_color : s_dark_tile_color;
    
    // Select highligthing
    bool is_selected = m_selected_tile.has_value() && m_selected_tile == tile;

    // Move highlighting
    bool is_previous_move_tile = false;
    auto last_move = m_game.get_last_move();
    if(last_move.has_value()){
        auto[from, to, promo] = Chess::uci_parse(last_move.value());
        is_previous_move_tile = from == tile || to == tile;
    }

    if (is_selected || is_previous_move_tile) { // alpha blending highlight
        color.r = (color.r * (255 - s_highlight_color.a) + s_highlight_color.r * s_highlight_color.a) / 255;
        color.g = (color.g * (255 - s_highlight_color.a) + s_highlight_color.g * s_highlight_color.a) / 255;
        color.b = (color.b * (255 - s_highlight_color.a) + s_highlight_color.b * s_highlight_color.a) / 255;
    }
    return color;
}

#include "gui/elements/board_view.hpp"
#include "gui/assets.hpp"

BoardView::BoardView(const Chess& game) : m_game(game) {
    _load_assets();
}

void BoardView::_set_on_move_callback(std::function<bool(const UCI&)> callback) {
    onMoveAttempt = std::move(callback);
}

void BoardView::set_position(const sf::Vector2f position) {
    m_position = position;
}

void BoardView::set_size(const float size) {
    m_size = size;
    m_tile_size = size / 8.f;
}

void BoardView::handle_event(const sf::Event& event) {
    m_move_applied = false;

    if (const auto& mouse_press_event = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouse_press_event->button == sf::Mouse::Button::Left) {
            _on_mouse_left_down(mouse_press_event->position);
        }
    }
    else if (const auto& mouse_released_event = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mouse_released_event->button == sf::Mouse::Button::Left) {
            _on_mouse_left_up(mouse_released_event->position);
        }
    }
    else if (const auto& mouse_move_event = event.getIf<sf::Event::MouseMoved>()) {
        _on_mouse_moved(mouse_move_event->position);
    }

    if(m_move_applied) {
        // Selection should be reset when a move was applied. 
        // This prevents the user from immediately dragging the just moved piece.
        m_selected_tile.reset();
    }
}

void BoardView::draw(sf::RenderWindow& window, bool is_human_turn) {
    _draw_board(window);

    // Stationary pieces
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            Chess::Tile tile(file, rank);
            if((m_is_dragging || m_promotion_prompt_active) && tile == m_selected_tile) continue;
            _draw_piece(window, m_game.get_piece_at(tile), _board_to_screen_space(tile));
        }
    }
    
    // Legal move highlights
    if(is_human_turn && m_selected_tile.has_value()){
        _draw_legal_moves(window, m_selected_tile.value());
    }

    // Promotion prompt
    if(m_promotion_prompt_active){
        _draw_promotion_prompt(window);
    }

    // Dragged piece
    if(m_is_dragging && m_selected_tile.has_value()){
        _draw_piece(window, m_game.get_piece_at(m_selected_tile.value()), m_drag_screen_position);
    }
}

void BoardView::_on_mouse_left_down(const sf::Vector2i& screen_position) {
    if(const auto tile = _screen_to_board_space(sf::Vector2f(screen_position))){
        if(m_promotion_prompt_active) {
            // Check if user pressed an option or not
            if(tile.value().file == m_promotion_prompt_tile.file && std::abs(tile.value().rank - m_promotion_prompt_tile.rank) <= 3) {
                // Make move with the promotion piece chosen from the list
                _on_piece_moved(m_selected_tile.value(), m_promotion_prompt_tile,
                                s_promotion_pieces[std::abs(tile.value().rank - m_promotion_prompt_tile.rank)]);
            }
            // In both cases the prompt should be closed
            m_promotion_prompt_active = false;
            m_selected_tile.reset();
        }
        else{
            // If there is a previous selection, make a move between these tiles
            if(m_selected_tile.has_value()){
                _on_piece_moved(m_selected_tile.value(), tile.value());
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

void BoardView::_on_mouse_left_up(const sf::Vector2i& screen_position){
    if(m_is_dragging){
        m_is_dragging = false;
        if(const auto tile = _screen_to_board_space(sf::Vector2f(screen_position))){
            // If there is a previous selection, make a move between these tiles
            if(m_selected_tile.has_value()){
                _on_piece_moved(m_selected_tile.value(), tile.value());
            }
        }
    }
}

void BoardView::_on_mouse_moved(const sf::Vector2i& screen_position) {
    if (m_is_dragging) {
        m_drag_screen_position = sf::Vector2f(screen_position);
    }
}

void BoardView::_on_piece_moved(const Chess::Tile& from, const Chess::Tile& to, const PieceType promotion) {
    if(promotion == PieceType::None){
        // Check if promotion prompt needs to be activated
        bool is_promotion_move = m_game.is_legal_move(Chess::uci_create(from, to, s_promotion_pieces[0]));
        if(is_promotion_move) {
            m_promotion_prompt_active = true;
            m_promotion_prompt_tile = to;
            return;
        }
    }

    UCI move = Chess::uci_create(from, to, promotion);
    m_move_applied = onMoveAttempt(move);
}


void BoardView::_draw_board(sf::RenderWindow& window) {
    sf::RectangleShape square(sf::Vector2f(m_tile_size, m_tile_size));
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            Chess::Tile tile(file, rank);
            square.setOrigin(sf::Vector2f(square.getSize()) / 2.0f);
            square.setPosition(_board_to_screen_space(tile));
            square.setFillColor(_get_tile_color(tile));
            window.draw(square);
        }
    }
}

void BoardView::_draw_promotion_prompt(sf::RenderWindow& window) {
    int dir = m_promotion_prompt_tile.rank == 7 ? -1 : 1;
    PlayerColor color = m_promotion_prompt_tile.rank == 7 ? PlayerColor::White : PlayerColor::Black;

    // Draw background
    sf::RectangleShape background;
    const float background_height = 4.5f;
    background.setFillColor(sf::Color(233, 233, 233, 255));
    background.setSize(sf::Vector2f(m_tile_size, m_tile_size * background_height));
    background.setOrigin(sf::Vector2f(background.getSize()) / 2.0f);
    background.setPosition(_board_to_screen_space(m_promotion_prompt_tile) - sf::Vector2f(0.f, m_tile_size * (background_height-1.f) / 2.f * dir));
    window.draw(background);

    // Draw pieces
    for (int i = 0; i < 4; i++) {
        Chess::Tile tile(m_promotion_prompt_tile.file, m_promotion_prompt_tile.rank + i * dir);
        _draw_piece(window, Piece(s_promotion_pieces[i], color), _board_to_screen_space(tile));
    }

    // Draw x icon
    sf::Sprite sprite(m_texture_x_icon);
    float scale = m_tile_size / sprite.getTexture().getSize().x * 0.2f; 
    sprite.setOrigin(sf::Vector2f(sprite.getTexture().getSize()) / 2.0f);
    sprite.setPosition(_board_to_screen_space(m_promotion_prompt_tile) - sf::Vector2f(0.f, m_tile_size * (background_height - 0.75f) * dir));
    sprite.setScale(sf::Vector2f(scale, scale));
    sprite.setColor(sf::Color(255, 255, 255, 195));
    window.draw(sprite);
}

void BoardView::_draw_legal_moves(sf::RenderWindow& window, const Chess::Tile& tile) {
    std::vector<UCI> moves(m_game.get_legal_moves());
    if (moves.empty()) return;

    PieceType selected_piece = m_game.get_piece_at(tile).type;
    for (const UCI& move : moves) {
        auto[from, to, promo] = Chess::uci_parse(move);
        if(from != tile) continue;

        if(promo != PieceType::None && promo != PieceType::Queen){
            // Avoid duplicating sprites on promotion moves. There are
            // multiple legal moves to this square differing only in promotion.
            continue; 
        }
        
        PieceType target_piece = m_game.get_piece_at(to).type;
        bool is_capture = (target_piece != PieceType::None)                             // normal capture
                        || (selected_piece == PieceType::Pawn && from.file != to.file); // en passant / pawn capture

        sf::Sprite sprite(is_capture ? m_texture_circle_hollow : m_texture_circle);
        float scale = m_tile_size / sprite.getTexture().getSize().x * (is_capture ? 0.95f : 0.3f); 

        sprite.setOrigin(sf::Vector2f(sprite.getTexture().getSize()) / 2.0f);
        sprite.setPosition(_board_to_screen_space(to));
        sprite.setScale(sf::Vector2f(scale, scale));
        sprite.setColor(sf::Color(255, 255, 255, 70));
        window.draw(sprite);
    }
}

void BoardView::_draw_piece(sf::RenderWindow& window, const Piece& piece, const sf::Vector2f& position) {
    if (piece.type == PieceType::None) return;

    sf::Sprite sprite(m_texture_pieces[piece]);

    sprite.setOrigin(sf::Vector2f(sprite.getTexture().getSize()) / 2.0f);
    sprite.setPosition(position);
    
    float scale = m_tile_size / sprite.getTexture().getSize().x;
    sprite.setScale(sf::Vector2f(scale, scale));

    window.draw(sprite);
}

sf::Color BoardView::_get_tile_color(const Chess::Tile& tile) {
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


void BoardView::_load_assets() {
    // Pieces
    for(PlayerColor color : {PlayerColor::White, PlayerColor::Black}){
        std::string prefix = (color == PlayerColor::White ? "w" : "b");
        m_texture_pieces[{PieceType::Pawn,   color}] = load_svg("pieces/" + prefix + "P.svg");
        m_texture_pieces[{PieceType::Knight, color}] = load_svg("pieces/" + prefix + "N.svg");
        m_texture_pieces[{PieceType::Bishop, color}] = load_svg("pieces/" + prefix + "B.svg");
        m_texture_pieces[{PieceType::Rook,   color}] = load_svg("pieces/" + prefix + "R.svg");
        m_texture_pieces[{PieceType::Queen,  color}] = load_svg("pieces/" + prefix + "Q.svg");
        m_texture_pieces[{PieceType::King,   color}] = load_svg("pieces/" + prefix + "K.svg");
    }

    // Other
    m_texture_circle = load_svg("circle.svg");
    m_texture_circle_hollow = load_svg("circle_hollow.svg");
    m_texture_x_icon = load_svg("x_icon.svg");
}

std::optional<Chess::Tile> BoardView::_screen_to_board_space(const sf::Vector2f& screen_pos) const {
    sf::FloatRect board_rect = sf::FloatRect(m_position, sf::Vector2f(m_size, m_size));
    if (!board_rect.contains(screen_pos)) return std::nullopt;

    int file = static_cast<int>((screen_pos.x - board_rect.position.x) / m_tile_size);    
    int rank = 7-static_cast<int>((screen_pos.y - board_rect.position.y) / m_tile_size);
    // safety check in case of rounding
    if (file < 0 || file >= 8 || rank < 0 || rank >= 8){
        return std::nullopt;
    }
    return Chess::Tile(file, rank);
}

sf::Vector2f BoardView::_board_to_screen_space(const Chess::Tile& tile) const {
    return sf::Vector2f(m_position.x + (tile.file + 0.5f) *  m_tile_size,
                        m_position.y + (7-tile.rank + 0.5f) * m_tile_size);
}

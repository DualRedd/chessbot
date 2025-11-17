#include "gui/chess_gui.hpp"
#include "ai/registry.hpp"

ChessGUI::ChessGUI(int window_width, int window_height)
    : m_game(),
      m_window(sf::VideoMode(sf::Vector2u(window_width, window_height)), "Chess"),
      m_tgui(m_window),
      m_side_panel(m_tgui),
      m_board_view(m_game)
{
    m_window.setFramerateLimit(60);

    // Center window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    int pos_x = (static_cast<int>(desktop.size.x) - window_width) / 2;
    int pos_y = (static_cast<int>(desktop.size.y) - window_height) / 2;
    m_window.setPosition(sf::Vector2i(pos_x, pos_y));

    // Callbacks
    m_board_view.on_move([this](const UCI& uci){
        return this->_on_gui_move(uci);
    });
    m_side_panel.on_new_game_pressed([this](){
        m_game.new_board();
        if(m_black_ai.has_value()){
            m_black_ai.value()->set_board(m_game.get_board_as_fen());
        }
        if(m_white_ai.has_value()){
            m_white_ai.value()->set_board(m_game.get_board_as_fen());
        }
    });
    m_side_panel.on_undo_pressed([this](){
        _try_undo_move();
    });

    // UI element setup
    _update_element_transforms();

    // configurable ai later
    m_black_ai = AIRegistry::create("Minimax"); 
    m_black_ai.value()->set_board(m_game.get_board_as_fen());
}

void ChessGUI::run() {
    while (m_window.isOpen()) {
        _handle_events();
        _handle_ai_moves();
        _draw();
    }
}

void ChessGUI::_handle_events() {
    bool resized = false;

    while (const std::optional event = m_window.pollEvent()) {
        m_tgui.handleEvent(*event);

        if (event->is<sf::Event::Closed>()){
            m_window.close();
        }
        else if (const auto& resize_event = event->getIf<sf::Event::Resized>()) {
            resized = true;
        }

        m_board_view.handle_event(event.value());
    }

    if(resized) _on_window_resize();
}

void ChessGUI::_handle_ai_moves() {
    auto& cur_ai = m_game.get_side_to_move() == PlayerColor::White ? m_white_ai : m_black_ai;
    if(!cur_ai.has_value()) return;

    if(!cur_ai.value()->is_computing() && !m_ai_move.has_value()) {
        // apply stored moves first to sync state
        auto& moves = m_game.get_side_to_move() == PlayerColor::White ? m_white_ai_unapplied_moves : m_black_ai_unapplied_moves;
        for(const UCI& move : moves) {
            if(move == "UNDO") cur_ai.value()->undo_move();
            else cur_ai.value()->apply_move(move);
        }
        moves.clear();

        // Start move calculation
        m_ai_move = cur_ai.value()->compute_move_async();
    }
    else if(m_ai_move.has_value() && m_ai_move.value()->done) {
        if (m_ai_move.value()->error){
            std::rethrow_exception(m_ai_move.value()->error);
        }
        if(!_try_make_move(m_ai_move.value()->result)){
            throw std::runtime_error(std::string("ChessGUI::_handle_ai_moves() - AI gave illegal move '") + m_ai_move.value()->result + "'!");
        }
        m_ai_move.reset();
    }
}

bool ChessGUI::_on_gui_move(const UCI& uci) {
    PlayerColor turn = m_game.get_side_to_move();
    if(turn == PlayerColor::White && m_white_ai != std::nullopt){
        return false;
    }
    if(turn == PlayerColor::Black && m_black_ai != std::nullopt){
        return false;
    }
    return _try_make_move(uci);
}

bool ChessGUI::_try_make_move(const UCI& move) {
    bool success = m_game.play_move(move);
    if(!success) return false;

    if(m_white_ai.has_value()) {
        m_white_ai_unapplied_moves.push_back(move);
    }
    if(m_black_ai.has_value()) {
        m_black_ai_unapplied_moves.push_back(move);
    }
    return true;
}

bool ChessGUI::_try_undo_move() {
    bool success = m_game.undo_move();
    if(!success) return false;

    m_ai_move.reset();
    if(m_white_ai.has_value()) {
        m_white_ai_unapplied_moves.push_back("UNDO");
    }
    if(m_black_ai.has_value()) {
        m_black_ai_unapplied_moves.push_back("UNDO");
    }
    return true;
}

void ChessGUI::_draw() {
    m_window.clear();

    bool is_human_turn = !m_ai_move.has_value();
    m_board_view.draw(m_window, is_human_turn);

    m_tgui.draw();

    m_window.display();
}

void ChessGUI::_on_window_resize() {
    sf::Vector2u size = m_window.getSize(); 
    sf::FloatRect rect(sf::Vector2f(0.0, 0.0), sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    m_window.setView(sf::View(rect));

    _update_element_transforms();
}

void ChessGUI::_update_element_transforms() {
    sf::Vector2f window_size = sf::Vector2f(m_window.getView().getSize());

    float side_panel_width = std::clamp(window_size.x * 0.4f, 200.f, 320.f);
    float board_size = std::min(window_size.x - side_panel_width, float(window_size.y));

    // Board
    sf::Vector2f board_offset((window_size.x - side_panel_width - board_size) / 2.f,
                              (window_size.y - board_size) / 2.f);
    m_board_view.set_position(board_offset);
    m_board_view.set_size(board_size);


    // Side panel
    m_side_panel.set_position(sf::Vector2f(
        board_offset.x + board_size - 0.1f,
        0
    ));
    m_side_panel.set_size(sf::Vector2f(
        side_panel_width,
        window_size.y
    ));

}

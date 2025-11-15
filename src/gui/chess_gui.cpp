#include "gui/chess_gui.hpp"
#include "ai/registry.hpp"

ChessGUI::ChessGUI(int window_width, int window_height)
    : m_window(sf::VideoMode(sf::Vector2u(window_width, window_height)), "Chess"),
      m_game(),
      m_board_view(m_game)
{
    m_window.setFramerateLimit(60);

    // Center window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    int pos_x = (static_cast<int>(desktop.size.x) - window_width) / 2;
    int pos_y = (static_cast<int>(desktop.size.y) - window_height) / 2;
    m_window.setPosition(sf::Vector2i(pos_x, pos_y));

    // Callbacks
    m_board_view._set_on_move_callback([this](const UCI& uci){
        return this->_on_gui_move(uci);
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
        if (event->is<sf::Event::Closed>()){
            m_window.close();
        }
        else if (const auto& resize_event = event->getIf<sf::Event::Resized>()) {
            resized = true;
        }

        else if (const auto& key_press_event = event->getIf<sf::Event::KeyPressed>()) {
            if (key_press_event->scancode == sf::Keyboard::Scan::R) {
                _try_undo_move(); /** DEBUGGING */
            }
        }

        m_board_view.handle_event(event.value());
    }

    if(resized) _on_window_resize();
}

void ChessGUI::_handle_ai_moves() {
    auto& cur_ai = m_game.get_side_to_move() == PlayerColor::White ? m_white_ai : m_black_ai;

    if(!m_ai_move.has_value() && cur_ai.has_value()) {
        m_ai_move = cur_ai.value()->compute_move_async();
    }

    if (m_ai_move.has_value() && m_ai_move.value()->done) {
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
        m_white_ai.value()->apply_move(move);
    }
    if(m_black_ai.has_value()) {
        m_black_ai.value()->apply_move(move);
    }
    return true;
}

bool ChessGUI::_try_undo_move() {
    bool success = m_game.undo_move();
    if(!success) return false;

    if(m_white_ai.has_value()) {
        m_white_ai.value()->undo_move();
    }
    if(m_black_ai.has_value()) {
        m_black_ai.value()->undo_move();
    }
    return true;
}

void ChessGUI::_draw() {
    m_window.clear();

    bool is_human_turn = !m_ai_move.has_value();
    m_board_view.draw(m_window, is_human_turn);

    m_window.display();
}

void ChessGUI::_on_window_resize() {
    sf::Vector2u size = m_window.getSize(); 
    sf::FloatRect rect(sf::Vector2f(0.0, 0.0), sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    m_window.setView(sf::View(rect));

    _update_element_transforms();
}

void ChessGUI::_update_element_transforms() {
    sf::Vector2u window_size = m_window.getSize();

    // Board
    float board_size_pixels = std::min(window_size.x, window_size.y);
    sf::Vector2f board_offset((window_size.x - board_size_pixels) / 2.f,
                              (window_size.y - board_size_pixels) / 2.f);
    m_board_view.set_position(board_offset);
    m_board_view.set_size(board_size_pixels);
}

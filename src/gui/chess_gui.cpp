#include "gui/chess_gui.hpp"

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
    m_board_view.setOnMoveAttemptCallback([this](const UCI& uci){
        return this->onUserMoveAttempt(uci);
    });

    // UI element setup
    _updateElementTransforms();
}

void ChessGUI::run() {
    while (m_window.isOpen()) {
        _handleEvents();
        _draw();
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

        else if (const auto& key_press_event = event->getIf<sf::Event::KeyPressed>()) {
            if (key_press_event->scancode == sf::Keyboard::Scan::R) {
                m_game.undo_move(); /** DEBUGGING */
            }
        }

        m_board_view.handleEvent(event.value());
    }

    if(resized) _onWindowResize();
}

bool ChessGUI::onUserMoveAttempt(const UCI& uci) {
    return m_game.play_move(uci);
}

void ChessGUI::_draw() {
    m_window.clear();
    m_board_view.draw(m_window);
    m_window.display();
}

void ChessGUI::_onWindowResize() {
    sf::Vector2u size = m_window.getSize(); 
    sf::FloatRect rect(sf::Vector2f(0.0, 0.0), sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    m_window.setView(sf::View(rect));

    _updateElementTransforms();
}

void ChessGUI::_updateElementTransforms() {
    sf::Vector2u window_size = m_window.getSize();

    // Board
    float board_size_pixels = std::min(window_size.x, window_size.y);
    sf::Vector2f board_offset((window_size.x - board_size_pixels) / 2.f,
                              (window_size.y - board_size_pixels) / 2.f);
    m_board_view.setPosition(board_offset);
    m_board_view.setSize(board_size_pixels);
}

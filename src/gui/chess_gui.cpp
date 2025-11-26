#include "gui/chess_gui.hpp"
#include "ai/registry.hpp"

ChessGUI::ChessGUI(int window_width, int window_height)
    : m_game_manager(),
      m_window(sf::VideoMode(sf::Vector2u(window_width, window_height)), "Chess"),
      m_tgui(m_window),
      m_side_panel(m_tgui),
      m_chess_view(m_game_manager)
{
    m_window.setFramerateLimit(60);

    // Center window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    int pos_x = (static_cast<int>(desktop.size.x) - window_width) / 2;
    int pos_y = (static_cast<int>(desktop.size.y) - window_height) / 2;
    m_window.setPosition(sf::Vector2i(pos_x, pos_y));

    // Callbacks
    m_side_panel.on_new_game_pressed([this]() {
        m_game_manager.new_game(
            m_side_panel.get_player_configuration(PlayerColor::White),
            m_side_panel.get_player_configuration(PlayerColor::Black),
            CHESS_START_POSITION
        );
    });
    m_side_panel.on_undo_pressed([this]() {
        m_game_manager.try_undo_move();
    });
    m_side_panel.on_flip_board_pressed([this]() {
        m_chess_view.flip_board();
    });
    m_side_panel.on_ai_speed_changed([this](double speed) {
        m_game_manager.set_ai_move_delay(speed);
    });

    // UI element setup
    _update_element_transforms();
}

ChessGUI::~ChessGUI() = default;

void ChessGUI::run() {
    while (m_window.isOpen()) {
        _handle_events();
        m_game_manager.update();
        _draw();
    }
}

void ChessGUI::_handle_events() {
    bool resized = false;

    while (const std::optional event = m_window.pollEvent()) {
        m_tgui.handleEvent(*event);

        if (event->is<sf::Event::Closed>()) {
            m_window.close();
        } else if (const auto& resize_event = event->getIf<sf::Event::Resized>()) {
            resized = true;
        }

        m_chess_view.handle_event(event.value());
    }

    if (resized) _on_window_resize();
}

void ChessGUI::_draw() {
    m_window.clear();

    m_tgui.draw();
    m_chess_view.draw(m_window);

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

    float side_panel_width = std::clamp(window_size.x * 0.4f, 200.f, 300.f);
    float board_size = std::min(window_size.x - side_panel_width, float(window_size.y));

    // Board
    sf::Vector2f board_offset((window_size.x - side_panel_width - board_size) / 2.f,
                              (window_size.y - board_size) / 2.f);
    m_chess_view.set_position(board_offset);
    m_chess_view.set_size(board_size);

    // Side panel
    m_side_panel.set_position(sf::Vector2f(board_offset.x + board_size - 0.1f, 0));
    m_side_panel.set_size(sf::Vector2f(side_panel_width, window_size.y));
}

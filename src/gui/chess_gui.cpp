#include "gui/chess_gui.hpp"

#include "gui/assets.hpp"

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
            m_side_panel.get_player_configuration(Color::White),
            m_side_panel.get_player_configuration(Color::Black),
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

    // Font
    if (!m_font.openFromFile(get_executable_dir() / "assets/fonts/Roboto-Regular.ttf")) {
        throw std::runtime_error("ChessGUI - Failed to load font!");
    }
}

ChessGUI::~ChessGUI() = default;

void ChessGUI::run() {
    while (m_window.isOpen()) {
        try {
            _handle_events();
            m_game_manager.update();
        }
        catch (const std::exception& e) {
            m_game_manager.end_game();
            m_error_popup_active = true;
            m_error_message = std::string("Error! ") + e.what();
        }
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

        // If popup active dont handle child events
        if (m_error_popup_active) {
            if (const auto& mouse_press_event = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouse_press_event->button == sf::Mouse::Button::Left) {
                    m_error_popup_active = false;
                }
            }
        }
        else {
            m_chess_view.handle_event(event.value());
        }
    }

    if (resized) _on_window_resize();
}

void ChessGUI::_draw() {
    m_window.clear();

    m_tgui.draw();
    m_chess_view.draw(m_window);

    if (m_error_popup_active) {
        _draw_error_popup(m_window);
    }

    m_window.display();
}

void ChessGUI::_draw_error_popup(sf::RenderWindow& window) {
    // Popup box
    sf::RectangleShape popup_box;
    popup_box.setFillColor(sf::Color(90, 6, 6));
    popup_box.setOutlineColor(sf::Color(180, 0, 0));
    popup_box.setOutlineThickness(4.f);

    // Text
    int popup_font_size = 24;
    sf::Text popup_text(m_font);
    popup_text.setCharacterSize(popup_font_size);
    popup_text.setString(m_error_message);
    popup_text.setFillColor(sf::Color::White);

    // Layout popup text and buttons
    sf::Vector2u size = m_window.getSize();
    const float padding = 28.f;
    float box_width = std::min(size.x - 10.f, popup_text.getLocalBounds().size.x + padding * 2.f);
    float box_height = std::min(size.y - 10.f, popup_text.getLocalBounds().size.y + padding * 2.f);

    // Scale text if needed
    while (popup_font_size > 1 && popup_text.getLocalBounds().size.x > box_width - 10.f) {
        popup_font_size -= 1;
        popup_text.setCharacterSize(popup_font_size);
        box_height = std::min(size.y - 10.f, popup_text.getLocalBounds().size.y + padding * 2.f);
    }

    sf::Vector2f center = sf::Vector2f(size.x * 0.5f, size.y * 0.5f);
    popup_box.setSize(sf::Vector2f(box_width, box_height));
    popup_box.setOrigin(popup_box.getSize() / 2.f);
    popup_box.setPosition(center);
    popup_text.setPosition(center - sf::Vector2f(popup_text.getLocalBounds().size.x / 2.f, popup_box.getSize().y / 2.f - padding + 1.f));

    // Draw popup
    window.draw(popup_box);
    window.draw(popup_text);
}


void ChessGUI::_on_window_resize() {
    sf::Vector2u size = m_window.getSize();
    sf::FloatRect rect(sf::Vector2f(0.0, 0.0), sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    m_window.setView(sf::View(rect));

    _update_element_transforms();
}

void ChessGUI::_update_element_transforms() {
    sf::Vector2f window_size = sf::Vector2f(m_window.getView().getSize());

    float wanted_side_panel_width = std::max(window_size.x * 0.3f, window_size.x - window_size.y);
    float side_panel_width = std::clamp(wanted_side_panel_width, 200.f, 440.f);
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

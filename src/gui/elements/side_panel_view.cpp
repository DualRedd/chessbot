#include "gui/elements/side_panel_view.hpp"

const tgui::Layout button_margin{"10.0"};
const tgui::Layout button_spacing{"1.5%"};
const tgui::Layout button_height{"40.0"}; 


SidePanelView::SidePanelView(tgui::Gui& gui)
  : m_panel(tgui::Panel::create()),
    m_undo_button(tgui::Button::create("Undo Move")),
    m_new_game_button(tgui::Button::create("New Game")),
    m_middle_scroll_panel(tgui::ScrollablePanel::create()),
    m_middle_grow_panel(tgui::GrowVerticalLayout::create()),
    m_black_player_config(m_middle_grow_panel, PlayerColor::Black),
    m_white_player_config(m_middle_grow_panel, PlayerColor::White)
{   
    // Main panel
    gui.add(m_panel);

    // Undo button
    m_panel->add(m_undo_button);
    m_undo_button->setPosition(button_margin, button_margin);
    m_undo_button->setSize("50%" - button_spacing - button_margin, button_height);
    std::weak_ptr<tgui::Button> weak_undo_button_ptr = m_undo_button; // weak pointer to avoid sptr cycle
    m_undo_button->onSizeChange([weak_undo_button_ptr](tgui::Vector2f size){
        if(auto btn = weak_undo_button_ptr.lock()) {
            btn->setTextSize(size.x * 0.14f);
        }
    });

    // New Game button
    m_panel->add(m_new_game_button);
    m_new_game_button->setPosition("50%" + button_spacing, button_margin);
    m_new_game_button->setSize("50%" - button_spacing - button_margin, button_height);
    std::weak_ptr<tgui::Button> weak_new_game_button_ptr = m_new_game_button; // weak pointer to avoid sptr cycle
    m_new_game_button->onSizeChange([weak_new_game_button_ptr](tgui::Vector2f size){
        if(auto btn = weak_new_game_button_ptr.lock()) {
            btn->setTextSize(size.x * 0.14f); 
        }
    });

    // Scroll panel
    m_panel->add(m_middle_scroll_panel);
    m_middle_scroll_panel->setPosition(0, button_height + 2 * button_margin);
    m_middle_scroll_panel->setSize("100%", "100%" - button_height - 2 * button_margin);
    m_middle_scroll_panel->getHorizontalScrollbar()->setPolicy(tgui::Scrollbar::Policy::Never);
    m_middle_scroll_panel->getVerticalScrollbar()->setPolicy(tgui::Scrollbar::Policy::Automatic);

    // TGUI use-after free bug workaround: Reserve a small width for the scrollbar
    // Scrollbar visibility changes due to GrowVerticalLayout size changes cause
    // GrowVerticalLayout to reenter its updateWidgets() from within ScrollablePanel's
    // updateWidget() which then trashes its m_widgetLayouts vector while it's still being iterated.
    m_middle_grow_panel->setSize("parent.width - 16", "100%");
    m_middle_scroll_panel->add(m_middle_grow_panel);
}

void SidePanelView::set_position(sf::Vector2f position) {
    m_panel->setPosition(tgui::Layout2d(position));
}

void SidePanelView::set_size(sf::Vector2f size) {
    m_panel->setSize(tgui::Layout2d(size));
}

void SidePanelView::on_undo_pressed(std::function<void()> callback) {
    m_undo_button->onPress(callback);
}

void SidePanelView::on_new_game_pressed(std::function<void()> callback) {
    m_new_game_button->onPress(callback);
}

PlayerConfiguration SidePanelView::get_player_configuration(PlayerColor side) {
    if (side == PlayerColor::Black) {
        return m_black_player_config.get_current_configuration();
    }
    else {
        return m_white_player_config.get_current_configuration();
    }
}

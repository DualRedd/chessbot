#include "gui/elements/side_panel_view.hpp"

SidePanelView::SidePanelView(tgui::Gui& gui) {
    m_panel = tgui::Panel::create();
    gui.add(m_panel);

    const float margin = 10.f;
    const float spacing = 5.f;
    const float buttonHeight = 30.f;

    // Undo button
    m_undo_button = tgui::Button::create("Undo Move");
    m_undo_button->setPosition(margin, margin);
    m_undo_button->setSize("50% - " + tgui::String(std::to_string(spacing + margin)), buttonHeight);
    std::weak_ptr<tgui::Button> weak_undo_button_ptr = m_undo_button; // weak pointer to avoid sptr cycle
    m_undo_button->onSizeChange([weak_undo_button_ptr](tgui::Vector2f size){
        if(auto btn = weak_undo_button_ptr.lock()) {
            btn->setTextSize(static_cast<uint>(size.x * 0.14f));
        }
    });
    m_panel->add(m_undo_button);

    // New Game button
    m_new_game_button = tgui::Button::create("New Game");
    m_new_game_button->setPosition("50% + " + tgui::String(std::to_string(spacing)), margin);
    m_new_game_button->setSize("50% - " + tgui::String(std::to_string(spacing + margin)), buttonHeight);
    std::weak_ptr<tgui::Button> weak_new_game_button_ptr = m_new_game_button; // weak pointer to avoid sptr cycle
    m_new_game_button->onSizeChange([weak_new_game_button_ptr](tgui::Vector2f size){
        if(auto btn = weak_new_game_button_ptr.lock()) {
            btn->setTextSize(static_cast<uint>(size.x * 0.14f));
        }
    });
    m_panel->add(m_new_game_button);
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

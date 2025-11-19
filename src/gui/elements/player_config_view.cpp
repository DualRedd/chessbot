#include "gui/elements/player_config_view.hpp"

PlayerConfigView::PlayerConfigView(tgui::Container::Ptr parent, tgui::Layout pos_y) {
    m_panel = tgui::GrowVerticalLayout::create();
    m_panel->setPosition(0, pos_y);
    //m_panel->setSize("100%", height);
    parent->add(m_panel);
}
 



#include "gui/elements/player_config_view.hpp"

const tgui::Layout dropdown_margin_left{"15.0"};
const tgui::Layout dropdown_margin_right{"5.0"};
const tgui::Layout dropdown_margin_vertical{"10.0"};
const tgui::Layout dropdown_height{"35.0"};
const tgui::Layout dropdown_label_width{"25%"};

const float text_size = 2.7f;

// Helper function
static inline std::string float_to_string(double val) {
    std::string s = std::to_string(val);
    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
    if (s.back() == '.') s.pop_back();
    return s;
}

PlayerConfigView::PlayerConfigView(tgui::GrowVerticalLayout::Ptr parent, Color color)
  : m_container(tgui::Panel::create()),
    m_dropdown(tgui::ComboBox::create())
{
    m_container->setSize("100%", dropdown_height + dropdown_margin_vertical * 2);
    parent->add(m_container);

    // Dropdown container
    auto dropdown_container = tgui::Panel::create();
    dropdown_container->setSize("100%", dropdown_height + dropdown_margin_vertical * 2);
    dropdown_container->setPosition(0, 0);

    // Dropdown label
    auto label = tgui::Label::create();
    label->setText(color == Color::White ? "White" : "Black");
    label->setPosition(dropdown_margin_left, dropdown_margin_vertical + 1);
    label->setSize(dropdown_label_width, dropdown_height);
    label->setHorizontalAlignment(tgui::HorizontalAlignment::Left);
    label->setVerticalAlignment(tgui::VerticalAlignment::Center);
    std::weak_ptr<tgui::Label> weak_label_ptr = label; // weak pointer to avoid sptr cycle
    label->onSizeChange([weak_label_ptr](tgui::Vector2f size) {
        if (auto lbl = weak_label_ptr.lock()) {
            lbl->setTextSize(std::min(size.x * text_size * 0.11f, size.y * text_size * 0.22f));
        }
    });

    // Dropdown itself
    m_dropdown->addItem("Human");
    m_dropdown->setSelectedItem("Human");
    for (auto& ai : AIRegistry::listAINames()) {
        m_dropdown->addItem(ai);
    }
    m_dropdown->setSize("100%" - dropdown_label_width - dropdown_margin_left - dropdown_margin_right, dropdown_height);
    m_dropdown->setPosition(dropdown_label_width + dropdown_margin_left, dropdown_margin_vertical);
    m_dropdown->getRenderer()->setPadding(8);
    std::weak_ptr<tgui::ComboBox> weak_dropdown_ptr = m_dropdown; // weak pointer to avoid sptr cycle
    m_dropdown->onSizeChange([weak_dropdown_ptr](tgui::Vector2f size) {
        if (auto drp = weak_dropdown_ptr.lock()) {
            drp->setTextSize(std::min(size.x * text_size * 0.1f, size.y * text_size * 0.17f));
        }
    });
    m_dropdown->onItemSelect([this](const tgui::String& selected) {
        _apply_item_selected(selected);
    });

    // Add dropdown container to main container
    dropdown_container->add(label);
    dropdown_container->add(m_dropdown);
    m_container->add(dropdown_container);

    // Pre-create per-AI panels and fields
    for (const auto& ai_name : AIRegistry::listAINames()) {
        auto fields = AIRegistry::listConfig(ai_name);

        std::vector<ConfigFieldView::Ptr> input_views;
        for (const auto& field : fields) {
            auto input_view = create_config_field_widget(field);
            m_container->add(input_view->get_container());
            input_view->get_container()->setVisible(false);
            input_views.push_back(input_view);
            m_all_field_views.push_back(input_view);
        }

        m_ai_field_views[ai_name] = std::move(input_views);
    }

    // initial recalculation
    _recalculate_internal_panel();
}

PlayerConfiguration PlayerConfigView::get_current_configuration() {
    PlayerConfiguration cfg;

    tgui::String selected = m_dropdown->getSelectedItem();
    cfg.is_human = (selected == "Human");

    if (cfg.is_human)
        return cfg; // no configuration for human players

    cfg.ai_name = selected.toStdString();
    auto it_inputs = m_ai_field_views.find(cfg.ai_name);
    if (it_inputs == m_ai_field_views.end()) {
        throw std::runtime_error("PlayerConfigView::get_current_configuration() - unknown selection: " + selected.toStdString());
    }

    const auto& inputs = it_inputs->second;
    for (size_t i = 0; i < inputs.size(); ++i) {
        ConfigField result = inputs[i]->get_state();
        cfg.ai_config.push_back(result);
    }

    return cfg;
}

void PlayerConfigView::_apply_item_selected(const tgui::String& selected) {
    // hide all field panels first
    for (auto& panel : m_all_field_views) {
        if (panel) panel->get_container()->setVisible(false);
    }

    // Human selected, no fields to show
    if (selected == "Human") {
        _recalculate_internal_panel();
        return;
    }

    // show panels for the selected AI
    auto it_containers = m_ai_field_views.find(selected.toStdString());
    if (it_containers != m_ai_field_views.end()) {
        for (auto& panel : it_containers->second) {
            if (panel) panel->get_container()->setVisible(true);
        }
    }

    _recalculate_internal_panel();
}

void PlayerConfigView::_recalculate_internal_panel() {
    // Stack visible children top-to-bottom inside m_container and compute required height.
    float y = dropdown_height.getValue() + (dropdown_margin_vertical * 2).getValue();

    // Position each field panel in the same order they were added
    for (auto& view : m_all_field_views) {
        if (!view) continue;
        auto container = view->get_container();
        if (container->isVisible()) {
            container->setPosition(0, y);
            y += container->getSize().y;
        } else {
            container->setPosition(0, 0);
        }
    }

    m_container->setSize("100%", tgui::Layout(y));
}
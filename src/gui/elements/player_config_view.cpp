#include "gui/elements/player_config_view.hpp"

const tgui::Layout config_widget_label_width{"50%"};
const tgui::Layout config_widget_height{"35.0"};
const tgui::Layout config_widget_margin_left{"20.0"};
const tgui::Layout config_widget_margin_right{"5.0"};
const tgui::Layout config_widget_margin_vertical{"5.0"};

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

PlayerConfigView::PlayerConfigView(tgui::GrowVerticalLayout::Ptr parent, PlayerColor color)
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
    label->setText(color == PlayerColor::White ? "White" : "Black");
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
        m_ai_fields[ai_name] = fields;

        std::vector<tgui::Widget::Ptr> input_widgets;
        std::vector<tgui::Widget::Ptr> field_panels;

        for (const auto& field : fields) {
            auto field_container = tgui::Panel::create();
            field_container->setSize("100%", config_widget_height + 2 * config_widget_margin_vertical);
            field_container->setVisible(false);

            // label
            auto label_field = tgui::Label::create();
            label_field->setSize(config_widget_label_width, "100%");
            label_field->setPosition(config_widget_margin_left, config_widget_margin_vertical);
            label_field->setText(field.description + ":");
            label_field->setHorizontalAlignment(tgui::HorizontalAlignment::Left);
            label_field->setVerticalAlignment(tgui::VerticalAlignment::Center);
            field_container->add(label_field);

            // input
            tgui::Widget::Ptr input;
            switch (field.type) {
                case FieldType::Bool: {
                    auto checkbox = tgui::CheckBox::create();
                    checkbox->setChecked(std::get<bool>(field.value));
                    checkbox->setSize("height", config_widget_height);
                    checkbox->setPosition("100% - height" - config_widget_margin_right, config_widget_margin_vertical);
                    input = checkbox;
                    break;
                }
                case FieldType::Int: {
                    auto edit = tgui::EditBox::create();
                    edit->setText(std::to_string(std::get<int>(field.value)));
                    edit->setSize("100%" - config_widget_label_width - config_widget_margin_left - config_widget_margin_right, config_widget_height);
                    edit->setPosition(config_widget_label_width + config_widget_margin_left, config_widget_margin_vertical);
                    edit->setInputValidator(tgui::EditBox::Validator::Int);
                    input = edit;
                    break;
                }
                case FieldType::Double: {
                    auto edit = tgui::EditBox::create();
                    edit->setText(float_to_string(std::get<double>(field.value)));
                    edit->setSize("100%" - config_widget_label_width - config_widget_margin_left - config_widget_margin_right, config_widget_height);
                    edit->setPosition(config_widget_label_width + config_widget_margin_left, config_widget_margin_vertical);
                    edit->setInputValidator(tgui::EditBox::Validator::Float);
                    input = edit;
                    break;
                }
                default:
                    continue;
            }

            field_container->add(input);
            m_container->add(field_container);

            input_widgets.push_back(input);
            field_panels.push_back(field_container);
            m_all_field_containers.push_back(field_container);
        }

        m_ai_field_inputs[ai_name] = std::move(input_widgets);
        m_ai_field_containers[ai_name] = std::move(field_panels);
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
    auto it_fields = m_ai_fields.find(cfg.ai_name);
    auto it_inputs = m_ai_field_inputs.find(cfg.ai_name);
    if (it_fields == m_ai_fields.end() || it_inputs == m_ai_field_inputs.end()) {
        throw std::runtime_error("PlayerConfigView::get_current_configuration() - unknown selection: " + selected.toStdString());
    }

    const auto& fields = it_fields->second;
    const auto& inputs = it_inputs->second;
    assert(inputs.size() == fields.size());

    for (size_t i = 0; i < fields.size(); i++) {
        tgui::Widget::Ptr widget = inputs[i];

        ConfigField result = fields[i];
        switch (fields[i].type) {
            case FieldType::Bool: {
                auto checkbox = widget->cast<tgui::CheckBox>();
                result.value = checkbox->isChecked();
                break;
            }
            case FieldType::Int: {
                auto edit = widget->cast<tgui::EditBox>();
                try {
                    result.value = std::stoi(edit->getText().toStdString());
                }
                catch (...) {
                    edit->setText("0");
                    result.value = 0;
                }
                break;
            }
            case FieldType::Double: {
                auto edit = widget->cast<tgui::EditBox>();
                try {
                    result.value = std::stod(edit->getText().toStdString());
                }
                catch (...) {
                    edit->setText("0.0");
                    result.value = 0.0;
                }
                break;
            }
            default:
                assert(false);
                break;
        }

        cfg.ai_config.push_back(result);
    }

    return cfg;
}

void PlayerConfigView::_apply_item_selected(const tgui::String& selected) {
    // hide all field panels first
    for (auto& panel : m_all_field_containers) {
        if (panel) panel->setVisible(false);
    }

    // Human selected, no fields to show
    if (selected == "Human") {
        _recalculate_internal_panel();
        return;
    }

    std::string name = selected.toStdString();
    auto it_containers = m_ai_field_containers.find(name);

    if (it_containers != m_ai_field_containers.end()) {
        // show panels for the selected AI
        for (auto& panel : it_containers->second) {
            if (panel) panel->setVisible(true);
        }
    }

    _recalculate_internal_panel();
}

void PlayerConfigView::_recalculate_internal_panel() {
    // Stack visible children top-to-bottom inside m_container and compute required height.
    float y = dropdown_height.getValue() + (dropdown_margin_vertical * 2).getValue();

    // Position each field panel in the same order they were added
    for (auto& container : m_all_field_containers) {
        if (!container) continue;
        if (container->isVisible()) {
            container->setPosition(0, y);
            y += (config_widget_height + 2 * config_widget_margin_vertical).getValue();
        } else {
            container->setPosition(0, 0);
        }
    }

    m_container->setSize("100%", tgui::Layout(y));
}
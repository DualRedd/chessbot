#include "gui/elements/config_widgets.hpp"
#include "gui/elements/style.hpp"

ConfigFieldView::ConfigFieldView(const ConfigField& field)
  : m_field(field),
    m_container(tgui::Panel::create()),
    m_label(tgui::Label::create())
{
    // container panel
    m_container->setSize("100%", config_widget_height + 2 * config_widget_margin_vertical);

    // label widget
    m_label->setSize(config_widget_label_width - config_widget_label_padding_right,
                    "100%" - 2 * config_widget_margin_vertical);
    m_label->setPosition(config_widget_margin_left, config_widget_margin_vertical);
    m_label->setText(field.description + ":");
    m_label->setHorizontalAlignment(tgui::HorizontalAlignment::Left);
    m_label->setVerticalAlignment(tgui::VerticalAlignment::Center);
    m_container->add(m_label);
} 

tgui::Widget::Ptr ConfigFieldView::get_container() {
    return m_container;
}

void ConfigFieldView::set_on_change_callback(std::function<void(const ConfigField&)> cb) {
    m_on_change = std::move(cb);
}

void ConfigFieldView::_notify_on_change() {
    if (m_on_change) {
        m_on_change(get_state());
    }
}

ConfigFieldView::Ptr create_config_field_widget(const ConfigField& field) {
    switch (field.type) {
        case FieldType::Bool:
            return std::make_shared<BoolFieldView>(field);
        case FieldType::Int:
            return std::make_shared<IntFieldView>(field);
        case FieldType::Double:
            return std::make_shared<DoubleFieldView>(field);
        case FieldType::String:
            return std::make_shared<StringFieldView>(field);
        default:
            throw std::invalid_argument("create_config_field_widget() - Unsupported ConfigField type!");
    }
}


BoolFieldView::BoolFieldView(const ConfigField& field)
  : ConfigFieldView(field)
{
    if(field.type != FieldType::Bool) {
        throw std::invalid_argument("BoolFieldView requires ConfigField of boolean type");
    }

    // checkbox widget
    m_checkbox = tgui::CheckBox::create();
    m_checkbox->setChecked(std::get<bool>(field.value));
    m_checkbox->onChange([this](bool){
        this->_notify_on_change();
    });

    m_checkbox->setSize("height", config_widget_height);
    m_checkbox->setPosition("100% - height" - config_widget_margin_right, config_widget_margin_vertical);
    m_container->add(m_checkbox);
}

ConfigField BoolFieldView::get_state() const {
    ConfigField out = m_field;
    out.value = m_checkbox->isChecked();
    return out;
}

void BoolFieldView::apply_default() {
    m_checkbox->setChecked(std::get<bool>(m_field.value));
}


IntFieldView::IntFieldView(const ConfigField& field)
    : ConfigFieldView(field)
{
    if(field.type != FieldType::Int) {
        throw std::invalid_argument("IntFieldView requires ConfigField of integer type");
    }

    m_edit = tgui::EditBox::create();
    m_edit->setInputValidator(tgui::EditBox::Validator::Int);
    m_edit->setText(std::to_string(std::get<int>(field.value)));
    m_edit->onTextChange([this](const tgui::String&){
        this->_notify_on_change();
    });

    m_edit->setSize("100%" - config_widget_label_width - config_widget_margin_left - config_widget_margin_right, config_widget_height);
    m_edit->setPosition(config_widget_label_width + config_widget_margin_left, config_widget_margin_vertical);
    m_container->add(m_edit);
}

ConfigField IntFieldView::get_state() const {
    ConfigField out = m_field;
    out.type = FieldType::Int;
    try {
        out.value = std::stoi(m_edit->getText().toStdString());
    } catch (...) {
        out.value = 0;
    }
    return out;
}

void IntFieldView::apply_default() {
    m_edit->setText(std::to_string(std::get<int>(m_field.value)));
}


// Helper function
static inline std::string float_to_string(double val) {
    std::string s = std::to_string(val);
    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
    if (s.back() == '.') s.pop_back();
    return s;
}

DoubleFieldView::DoubleFieldView(const ConfigField& field)
    : ConfigFieldView(field)
{
    if(field.type != FieldType::Double) {
        throw std::invalid_argument("DoubleFieldView requires ConfigField of double type");
    }

    m_edit = tgui::EditBox::create();
    m_edit->setInputValidator(tgui::EditBox::Validator::Float);
    m_edit->setText(float_to_string(std::get<double>(field.value)));
    m_edit->onTextChange([this](const tgui::String&){
        this->_notify_on_change();
    });

    m_edit->setSize("100%" - config_widget_label_width - config_widget_margin_left - config_widget_margin_right, config_widget_height);
    m_edit->setPosition(config_widget_label_width + config_widget_margin_left, config_widget_margin_vertical);
    m_container->add(m_edit);
}

ConfigField DoubleFieldView::get_state() const {
    ConfigField out = m_field;
    out.type = FieldType::Double;
    try {
        out.value = std::stod(m_edit->getText().toStdString());
    } catch (...) {
        out.value = 0.0;
    }
    return out;
}

void DoubleFieldView::apply_default() {
    m_edit->setText(float_to_string(std::get<double>(m_field.value)));
}

StringFieldView::StringFieldView(const ConfigField& field)
    : ConfigFieldView(field)
{
    if(field.type != FieldType::String) {
        throw std::invalid_argument("StringFieldView requires ConfigField of string type");
    }

    m_edit = tgui::EditBox::create();
    m_edit->setText(std::get<std::string>(field.value));
    m_edit->onTextChange([this](const tgui::String&){
        this->_notify_on_change();
    });

    m_edit->setSize("100%" - config_string_widget_label_width - config_widget_margin_left - config_widget_margin_right, config_widget_height);
    m_edit->setPosition(config_string_widget_label_width + config_widget_margin_left, config_widget_margin_vertical);
    m_container->add(m_edit);

    // adjust label size for this specialization
    m_label->setSize(config_string_widget_label_width - config_widget_label_padding_right,
                    "100%" - 2 * config_widget_margin_vertical);
}

ConfigField StringFieldView::get_state() const {
    ConfigField out = m_field;
    out.type = FieldType::String;
    out.value = m_edit->getText().toStdString();
    return out;
}

void StringFieldView::apply_default() {
    m_edit->setText(std::get<std::string>(m_field.value));
}

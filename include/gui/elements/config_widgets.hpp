#pragma once

#include <functional>

#include "TGUI/TGUI.hpp"
#include "../player_configuration.hpp"

/**
 * Abstract wrapper class to encapsulate creation and value retrieval for
 * configuration input widgets (checkbox, editbox, ...).
 */
class ConfigFieldView {
public:
    using Ptr = std::shared_ptr<ConfigFieldView>;

    /**
     * @param field ConfigField describing the field to be represented by this widget
     */
    explicit ConfigFieldView(const ConfigField& field);
    virtual ~ConfigFieldView() = default;

    /**
     * @return pointer to the underlying containter widget
     */
    tgui::Widget::Ptr get_container();

    /**
     * @param cb callback to call on value change with the current state as parameter
     */
    void set_on_change_callback(std::function<void(const ConfigField&)> cb);

    /**
     * @return ConfigField representing the current state of the widget
     */
    virtual ConfigField get_state() const = 0;

    /**
     * Set the value of the widget to its default.
     */
    virtual void apply_default() = 0;

protected:
    /** Call callback if set */
    void _notify_on_change();

protected:
    ConfigField m_field;
    tgui::Panel::Ptr m_container;
    tgui::Label::Ptr m_label;
    std::function<void(const ConfigField&)> m_on_change{nullptr};
};

/**
 * Factory function to create appropriate ConfigFieldWidget subclass
 * based on the type of the given ConfigField.
 * @return pointer to the created ConfigFieldView
 */
ConfigFieldView::Ptr create_config_field_widget(const ConfigField& field);


class BoolFieldView : public ConfigFieldView {
public:
    explicit BoolFieldView(const ConfigField& field);
    ConfigField get_state() const override;
    void apply_default() override;

private:
    tgui::CheckBox::Ptr m_checkbox;
};

class IntFieldView : public ConfigFieldView {
public:
    explicit IntFieldView(const ConfigField& field);
    ConfigField get_state() const override;
    void apply_default() override;

private:
    tgui::EditBox::Ptr m_edit;
};

class DoubleFieldView : public ConfigFieldView {
public:
    explicit DoubleFieldView(const ConfigField& field);
    ConfigField get_state() const override;
    void apply_default() override;

private:
    tgui::EditBox::Ptr m_edit;
};

class StringFieldView : public ConfigFieldView {
public:
    explicit StringFieldView(const ConfigField& field);
    ConfigField get_state() const override;
    void apply_default() override;

private:
    tgui::EditBox::Ptr m_edit;
};




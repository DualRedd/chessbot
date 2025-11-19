#pragma once

#include "TGUI/TGUI.hpp"
#include "TGUI/Backend/SFML-Graphics.hpp"

/**
 * TODO
 */
class PlayerConfigView {
public:
    /**
     * TODO
     * @param gui TGUI object this view will be part of
     */
    PlayerConfigView(tgui::Container::Ptr parent, tgui::Layout pos_y);


    /*void setAI(const std::string& ai_name)
    {
        panel->removeAllWidgets(); // Clear previous widgets
        m_ai_name = ai_name;
        m_fields = AIRegistry::listConfig(ai_name);

        float y = 10.f;
        float padding = 10.f;

        for (auto& field : m_fields)
        {
            switch (field.type)
            {
                case FieldType::Bool: {
                    auto cb = tgui::CheckBox::create(field.name);
                    cb->setPosition(10, y);
                    cb->setChecked(std::get<bool>(field.value));
                    panel->add(cb);
                    m_widgets[field.name] = cb;
                    y += cb->getSize().y + padding;
                    break;
                }
                case FieldType::Int:
                case FieldType::Float:
                case FieldType::String: {
                    auto eb = tgui::EditBox::create();
                    eb->setSize(200, 30);
                    eb->setPosition(10, y);
                    eb->setText(std::visit([](auto&& v){ return std::to_string(v); }, field.value));
                    panel->add(eb);
                    m_widgets[field.name] = eb;
                    y += 30 + padding;
                    break;
                }
            }
        }
    }

    std::vector<ConfigField> getCurrentConfig() const
    {
        std::vector<ConfigField> cfg;
        for (auto& field : m_fields)
        {
            auto it = m_widgets.find(field.name);
            if (!it) continue;

            ConfigField current = field;

            switch (field.type)
            {
                case FieldType::Bool:
                    current.value = std::dynamic_pointer_cast<tgui::CheckBox>(it->second)->isChecked();
                    break;
                case FieldType::Int:
                    current.value = std::stoi(std::dynamic_pointer_cast<tgui::EditBox>(it->second)->getText().toStdString());
                    break;
                case FieldType::Float:
                    current.value = std::stod(std::dynamic_pointer_cast<tgui::EditBox>(it->second)->getText().toStdString());
                    break;
                case FieldType::String:
                    current.value = std::dynamic_pointer_cast<tgui::EditBox>(it->second)->getText().toStdString();
                    break;
            }

            cfg.push_back(current);
        }
        return cfg;
    }*/

private:
    tgui::GrowVerticalLayout::Ptr m_panel;
    std::unordered_map<std::string, tgui::Widget::Ptr> m_widgets;
};

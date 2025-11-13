#pragma once

#include "../core/ai_player.hpp"

#include <functional>
#include <memory>
#include <variant>
#include <stdexcept>

enum class FieldType { Bool, Int, Float, String };
struct ConfigField {
    std::string name;
    FieldType type;
    std::variant<bool, int, double, std::string> value;
};

template<typename T>
T getConfigValue(const std::vector<ConfigField>& cfg, const std::string& name) {
    for (const auto& field : cfg) {
        if (field.name == name) return std::get<T>(field.value);
    }
    throw std::invalid_argument("getConfigValue() - missing field: " + name);
}

class AIRegistry {
public:
    // Factory function definition. Build a AIPlayer from given configuration.
    using Factory = std::function<std::unique_ptr<AIPlayer>(const std::vector<ConfigField>&)>;

    /**
     * Add an AI to the registry.
     * @param name name for the AI
     * @param fields configuration fields for this AI
     * @param factory factory function for constructing this AI from given configuration values
     */
    static void registerAI(std::string name, std::vector<ConfigField> fields, Factory factory);

    /**
     * Create an AI with default configuration values.
     * @param name name of the ai
     * @throw std::invalid_argument if the name is not found within the registry
     */
    static std::unique_ptr<AIPlayer> create(const std::string& name);

    /**
     * Create an AI.
     * @param name name of the AI
     * @param cfg configuration values
     * @throw std::invalid_argument if the name is not found within the registry
     */
    static std::unique_ptr<AIPlayer> create(const std::string& name, const std::vector<ConfigField>& cfg);

    /**
     * @return Names of available AIs.
     */
    static std::vector<std::string> listAINames();

    /**
     * @param name name of the AI
     * @return Configuration fields associated with this AI
     * @throw std::invalid_argument if the name is not found within the registry
     */
    static std::vector<ConfigField> listConfig(const std::string& name);

private:
    // Registry entry
    struct Entry {
        Factory factory;
        std::vector<ConfigField> fields;
    };

    // The registry
    static std::unordered_map<std::string, Entry>& registry() {
        static std::unordered_map<std::string, Entry> instance;
        return instance;
    }

private:
    // Registers AIs to this registry
    static void registerAIs();

    // Static self-initialization of the registry
    struct Initializer {
        Initializer() {
            registerAIs();
        }
    };
    static inline Initializer _init_once;
};

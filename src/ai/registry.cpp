#include "ai/registry.hpp"

#include "ai/ai_random.hpp"
#include "ai/ai_minimax.hpp"

void AIRegistry::registerAIs() {
    registerRandomAI();
    registerMinimaxAI();
}

void AIRegistry::registerAI(std::string name, std::vector<ConfigField> fields, Factory factory) {
    registry().emplace(std::move(name), Entry{std::move(factory), std::move(fields)});
}

std::unique_ptr<AIPlayer> AIRegistry::create(const std::string& name) {
    return AIRegistry::create(name, AIRegistry::listConfig(name));
}

std::unique_ptr<AIPlayer> AIRegistry::create(const std::string& name, const std::vector<ConfigField>& cfg) {
    if (registry().find(name) == registry().end()) {
        throw std::invalid_argument("AIRegistry::create() - invalid type!");
    }
    return registry()[name].factory(cfg);
}

std::vector<std::string> AIRegistry::listAINames() {
    std::vector<std::string> result;
    for (const auto& [type, entry] : registry())
        result.push_back(type);
    return result;
}

std::vector<ConfigField> AIRegistry::listConfig(const std::string& name) {
    if (registry().find(name) == registry().end()) {
        throw std::invalid_argument("AIRegistry::listConfig() - invalid type!");
    }
    return registry().at(name).fields;
}

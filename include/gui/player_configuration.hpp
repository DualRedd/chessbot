#pragma once

#include "core/registry.hpp"

/**
 * Represents a human player or a AI player with a configuration.
 */
struct PlayerConfiguration {
    bool is_human;
    std::string ai_name;
    std::vector<ConfigField> ai_config;
};

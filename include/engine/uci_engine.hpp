#pragma once

#include <random>

#include "registry.hpp"
#include "../core/position.hpp"
#include "../core/move_generation.hpp"

void registerRandomAI();

class RandomAI : public AIPlayer {
public:
    explicit RandomAI(const std::vector<ConfigField>& cfg);

private:
    RandomAI() = delete;
    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI _compute_move() override;

    Position m_board;
    MoveList m_move_list;
    std::mt19937 m_rng;
};

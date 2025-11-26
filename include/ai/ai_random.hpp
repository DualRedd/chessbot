#pragma once

#include <random>

#include "registry.hpp"
#include "../core/board.hpp"

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

    Board m_board;
    std::mt19937 m_rng;
};

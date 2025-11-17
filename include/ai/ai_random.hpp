#pragma once

#include "registry.hpp"
#include "../core/board.hpp"

void registerRandomAI();

class RandomAI : public AIPlayer {
public:
    RandomAI() = default;

    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI _compute_move() override;

private:
    Board m_board;
};

#pragma once

#include "registry.hpp"
#include "../core/board.hpp"

void registerRandomAI();

class RandomAI : public AIPlayer {
public:
    RandomAI() = default;

    virtual void set_board(const FEN& fen) override;
    virtual void apply_move(const UCI& move) override;
    virtual void undo_move() override;
    virtual UCI compute_move() override;

private:
    Board m_board;
};

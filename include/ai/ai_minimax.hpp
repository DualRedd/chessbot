#pragma once

#include "registry.hpp"
#include "search_position.hpp"

void registerMinimaxAI();

class MinimaxAI : public AIPlayer {
public:
    MinimaxAI() = default;

    virtual void set_board(const FEN& fen) override;
    virtual void apply_move(const UCI& move) override;
    virtual void undo_move() override;
    virtual UCI compute_move() override;

private:
    int _alphaBeta(int alpha, int beta, int depth_left);

private:
    SearchPosition m_position;
    int m_search_depth = 5;
};

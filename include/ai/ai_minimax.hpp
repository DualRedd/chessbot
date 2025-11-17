#pragma once

#include "registry.hpp"
#include "search_position.hpp"

void registerMinimaxAI();

class MinimaxAI : public AIPlayer {
public:
    MinimaxAI() = default;

    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI _compute_move() override;

private:
    int _alpha_beta(int alpha, int beta, int depth_left);

private:
    SearchPosition m_position;
    int m_search_depth = 5;
};

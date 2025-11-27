#pragma once

#include "registry.hpp"
#include "search_position.hpp"
#include "transposition_table.hpp"

void registerMinimaxAI();

class MinimaxAI : public AIPlayer {
public:
    MinimaxAI() = default;

private:
    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI _compute_move() override;
    int32_t _alpha_beta(int32_t alpha, int32_t beta, int depth, int ply);

private:
    SearchPosition m_position;
    TranspositionTable m_tt;
    int m_search_depth = 5;
};

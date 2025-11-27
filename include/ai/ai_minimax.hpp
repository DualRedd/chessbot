#pragma once

#include "registry.hpp"
#include "search_position.hpp"
#include "transposition_table.hpp"

void registerMinimaxAI();

class MinimaxAI : public AIPlayer {
public:
    MinimaxAI();

private:
    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI _compute_move() override;

    std::pair<int32_t, Move> _root_search(int32_t alpha, int32_t beta);
    int32_t _alpha_beta(int32_t alpha, int32_t beta, int depth, int ply);
    inline void _order_moves(std::vector<Move>& moves, const TTEntry* tt_entry) const;

private:
    SearchPosition m_position;
    TranspositionTable m_tt;
    int m_search_depth = 7;
};

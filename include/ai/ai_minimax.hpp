#pragma once

#include <chrono>
#include "registry.hpp"
#include "search_position.hpp"
#include "transposition_table.hpp"
#include "../core/move_generation.hpp"

void registerMinimaxAI();

class MinimaxAI : public AIPlayer {
public:
    MinimaxAI(const std::vector<ConfigField>& cfg);

private:
    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI _compute_move() override;

    std::pair<int32_t, Move> _root_search(int32_t alpha, int32_t beta, int32_t search_depth);
    int32_t _alpha_beta(int32_t alpha, int32_t beta, int depth, int ply);
    inline int32_t _quiescence(int32_t alpha, int32_t beta, int ply);
    inline void _order_moves(std::vector<Move>& moves, const TTEntry* tt_entry) const;
    inline bool _timer_check();

private:
    SearchPosition m_search_position;
    MoveList m_move_list;
    TranspositionTable m_tt;

    // Search parameters
    const double m_time_limit_seconds = 5.0;
    const int32_t m_aspiration_window = 50;

    // Timed cutoff
    std::chrono::steady_clock::time_point m_deadline;
    uint32_t m_nodes_visited = 0;
    bool m_stop_search = false;
};

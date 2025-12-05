#pragma once

#include <chrono>
#include <iostream>

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
    inline void _order_moves(MoveList& move_list, const TTEntry* tt_entry) const;
    inline bool _timer_check();

private:
    SearchPosition m_search_position;
    //MoveList m_move_list;
    TranspositionTable m_tt;

    // Search parameters
    const double m_time_limit_seconds = 5.0;
    bool m_aspiration_window_enabled = true;
    const int32_t m_aspiration_window = 50;

    // Timed cutoff
    std::chrono::steady_clock::time_point m_deadline;
    uint32_t m_nodes_visited = 0;
    bool m_stop_search = false;

    struct Stats {
        uint32_t depth = 0;
        uint32_t alpha_beta_nodes = 0;
        uint32_t quiescence_nodes = 0;
        uint32_t aspiration_misses = 0;
        uint32_t aspiration_miss_nodes = 0;
        uint64_t tt_raw_hits = 0;
        uint64_t tt_usable_hits = 0;
        uint32_t tt_cutoffs = 0;
        int32_t eval = 0;
        void reset() {
            depth = 0;
            alpha_beta_nodes = 0;
            quiescence_nodes = 0;
            aspiration_misses = 0;
            aspiration_miss_nodes = 0;
            tt_raw_hits = 0;
            tt_usable_hits = 0;
            tt_cutoffs = 0;
            eval = 0;
        }
        void print() const {
            std::cout << "Stats:\n";
            std::cout << "   Search depth: " << depth << "\n";
            std::cout << "   Best move eval: " << eval << "\n";
            std::cout << "   Alpha-Beta nodes: " << alpha_beta_nodes << "\n";
            std::cout << "   Quiescence nodes: " << quiescence_nodes << "\n";
            std::cout << "   Aspiration misses: " << aspiration_misses << "\n";
            std::cout << "   Aspiration miss nodes: " << aspiration_miss_nodes << "\n";
            std::cout << "   TT raw hit %: " << (double)tt_raw_hits / (double)alpha_beta_nodes * 100.0 << "\n";
            std::cout << "   TT usable hit %: " << (double)tt_usable_hits / (double)alpha_beta_nodes * 100.0 << "\n";
            std::cout << "   TT cutoff %: " << (double)tt_cutoffs / (double)alpha_beta_nodes * 100.0 << "\n";
        }
    } m_stats;

};

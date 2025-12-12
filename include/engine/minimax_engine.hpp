#pragma once

#include <chrono>

#include "../core/move_generation.hpp"
#include "../core/registry.hpp"
#include "search_position.hpp"
#include "transposition_table.hpp"

void registerMinimaxAI();

class MinimaxAI : public AIPlayer {
public:
    MinimaxAI(const std::vector<ConfigField>& cfg);
    MinimaxAI(const int32_t max_depth,
              const double time_limit_seconds,
              const size_t tt_size_megabytes,
              const bool aspiration_window_enabled,
              const int32_t aspiration_window,
              const bool enable_info_output = true);

    void set_time_limit_seconds(double secs);
    void set_max_depth(int depth);
    void set_max_nodes(int64_t nodes);
    void clear_transposition_table();

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
        double time_seconds = 0.0;
        void reset();
        void print() const;
    };

    Stats get_stats() const;

private:
    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI _compute_move() override;

    std::pair<int32_t, Move> _root_search(int32_t alpha, int32_t beta, int32_t search_depth, Move previous_best, bool info_output);
    int32_t _alpha_beta(int32_t alpha, int32_t beta, int32_t depth, int32_t ply);
    inline int32_t _quiescence(int32_t alpha, int32_t beta, int32_t ply);
    inline bool _stop_check();

private:
    // Search parameters
    int32_t m_max_depth = 99;
    double m_time_limit_seconds = 5.0;
    int64_t m_max_nodes = std::numeric_limits<int64_t>::max();

    const size_t m_tt_size_megabytes = 256ULL;
    const bool m_aspiration_window_enabled = true;
    const int32_t m_aspiration_window = 50;

    // Search state
    SearchPosition m_search_position;
    TranspositionTable m_tt;

    // Timed/node cutoff
    std::chrono::steady_clock::time_point m_deadline;
    int64_t m_nodes_visited = 0;
    bool m_stop_search = false;

    // Statistics
    const bool m_enable_info_output = true;
    Stats m_stats;
};

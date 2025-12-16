#pragma once

#include <chrono>

#include "../core/registry.hpp"
#include "search_position.hpp"
#include "transposition_table.hpp"
#include "history_tables.hpp"

void registerMinimaxAI();

enum class NodeType { Root, PV, NonPV };

class MinimaxAI : public AIPlayer {
public:
    MinimaxAI(const std::vector<ConfigField>& cfg);
    MinimaxAI(const int32_t max_depth,
              const double time_limit_seconds,
              const size_t tt_size_megabytes,
              const bool enable_uci_output = true);
    
    /**
     * Set time limit for the search in seconds.
     * @param secs time limit in seconds. Use a negative value for no limit.
     */
    void set_time_limit_seconds(double secs);

    /**
     * Set maximum search depth.
     * @param depth the depth. Use a negative value for no limit.
     */
    void set_max_depth(int depth);

    /**
     * Set maximum number of nodes to search.
     * @param nodes the amount of nodes. Use a negative value for no limit.
     */
    void set_max_nodes(int64_t nodes);

    /**
     * Clear the transposition table.
     */
    void clear_transposition_table();

    /**
     * Mate finding utility.
     * @return Pair of (mate in N moves, first move). The mate distance is positive for side to move
     * delivering mate, negative for side to move being mated. If no mate is found, the distance returned is 0.
     * @note Should limit running time using node count or time limit before calling this method.
     */
    std::pair<int, UCI> find_mate();

public:
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

    // Get statistics from the last search
    Stats get_stats() const;

private:
    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI _compute_move() override;

    // Alpha-beta search
    template<NodeType node_type>
    int32_t _alpha_beta(int32_t alpha, int32_t beta, const int32_t depth, const int32_t ply, const int32_t prior_reductions = 0);

    // Quiescence search
    inline int32_t _quiescence(int32_t alpha, int32_t beta, const int32_t ply);

    // True if search should stop (time/node limit reached or stop requested)
    inline bool _stop_check();

private:
    // Search parameters
    int32_t m_max_depth = 99;
    double m_time_limit_seconds = 5.0;
    int64_t m_max_nodes = std::numeric_limits<int64_t>::max();
    const size_t m_tt_size_megabytes = 256ULL;

    // Search state
    SearchPosition m_spos;
    TranspositionTable m_tt;
    KillerHistory m_killer_history;
    MoveHistory m_move_history;

    Move m_root_best_move;
    int32_t m_root_best_score;
    int32_t m_seldepth;

    // Timed/node cutoff
    int32_t m_start_time;
    int32_t m_deadline;
    int64_t m_nodes_visited = 0;
    bool m_stop_search = false;

    // Statistics
    const bool m_enable_uci_output = true;
    Stats m_stats;
};

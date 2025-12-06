#include "ai/ai_minimax.hpp"
#include <iostream>

static constexpr int32_t NULL_SCORE = 111'111'111;
static constexpr int32_t INF_SCORE = 100'000'000;
static constexpr int32_t MATE_SCORE = 100'000;
static constexpr int32_t DRAW_SCORE = 0;

static inline int32_t normalize_score_for_tt(int32_t score, int ply) {
    if (score > MATE_SCORE - 1000)
        return score + ply;
    if (score < -MATE_SCORE + 1000)
        return score - ply;
    return score;
}

static inline int32_t adjust_score_from_tt(int32_t stored_score, int ply) {
    if (stored_score > MATE_SCORE - 1000)
        return stored_score - ply;
    if (stored_score < -MATE_SCORE + 1000)
        return stored_score + ply;
    return stored_score;
}

void registerMinimaxAI() {
    auto createMinimaxAI = [](const std::vector<ConfigField>& cfg) {
        return std::make_unique<MinimaxAI>(cfg);
    };

    std::vector<ConfigField> cfg = {
        {"time_limit", "Thinking time (s)", FieldType::Double, 5.0},
        {"aspiration_window_enabled", "Enable aspiration window", FieldType::Bool, true},
        {"aspiration_window", "Aspiration window (centipawns)", FieldType::Int, 50}
    };

    AIRegistry::registerAI("Minimax", cfg, createMinimaxAI);
}

MinimaxAI::MinimaxAI(const std::vector<ConfigField>& cfg)
  : m_time_limit_seconds(get_config_field_value<double>(cfg, "time_limit")),
    m_aspiration_window_enabled(get_config_field_value<bool>(cfg, "aspiration_window_enabled")),
    m_aspiration_window(get_config_field_value<int>(cfg, "aspiration_window")),
    m_search_position(),
    m_tt(m_tt_size_megabytes)
{}

MinimaxAI::MinimaxAI(const int32_t max_depth,
                    const double time_limit_seconds,
                    const size_t tt_size_megabytes,
                    const bool aspiration_window_enabled,
                    const int32_t aspiration_window,
                    const bool enable_debug_output) 
  : m_max_depth(max_depth),
    m_time_limit_seconds(time_limit_seconds),
    m_tt_size_megabytes(tt_size_megabytes),
    m_aspiration_window_enabled(aspiration_window_enabled),
    m_aspiration_window(aspiration_window),
    m_search_position(),
    m_tt(m_tt_size_megabytes),
    m_enable_debug_output(enable_debug_output)
{}

void MinimaxAI::Stats::reset() {
    depth = 0;
    alpha_beta_nodes = 0;
    quiescence_nodes = 0;
    aspiration_misses = 0;
    aspiration_miss_nodes = 0;
    tt_raw_hits = 0;
    tt_usable_hits = 0;
    tt_cutoffs = 0;
    eval = 0;
    time_seconds = 0.0;
}
void MinimaxAI::Stats::print() const {
    std::cout << "Stats:\n";
    std::cout << "   Search depth: " << depth << "\n";
    std::cout << "   Best move eval: " << eval << "\n";
    std::cout << "   Alpha-Beta nodes: " << alpha_beta_nodes << "\n";
    std::cout << "   Alpha-Beta nps: " << (double)alpha_beta_nodes / time_seconds << "\n";
    std::cout << "   Quiescence nodes: " << quiescence_nodes << "\n";
    std::cout << "   Quiescence nps: " << (double)quiescence_nodes / time_seconds << "\n";
    std::cout << "   Aspiration misses: " << aspiration_misses << "\n";
    std::cout << "   Aspiration miss nodes: " << aspiration_miss_nodes << "\n";
    std::cout << "   TT raw hit %: " << (double)tt_raw_hits / (double)alpha_beta_nodes * 100.0 << "\n";
    std::cout << "   TT usable hit %: " << (double)tt_usable_hits / (double)alpha_beta_nodes * 100.0 << "\n";
    std::cout << "   TT cutoff %: " << (double)tt_cutoffs / (double)alpha_beta_nodes * 100.0 << "\n";
}

auto MinimaxAI::get_stats() const -> Stats {
    return m_stats;
}

void MinimaxAI::_set_board(const FEN& fen) {
    m_search_position.set_board(fen);
}

void MinimaxAI::_apply_move(const UCI& uci_move) {
    MoveList move_list;
    Move move = m_search_position.get_position().move_from_uci(uci_move);
    move_list.generate<GenerateType::Legal>(m_search_position.get_position());
    if (std::find(move_list.begin(), move_list.end(), move) == move_list.end()) {
        throw std::invalid_argument("MinimaxAI::apply_move() - illegal move!");
    }
    m_search_position.make_move(move);
}

void MinimaxAI::_undo_move() {
    if (!m_search_position.undo_move()) {
        throw std::invalid_argument("MinimaxAI::undo_move() - no previous move!");
    }
}

UCI MinimaxAI::_compute_move() {
    MoveList move_list;
    move_list.generate<GenerateType::Legal>(m_search_position.get_position());
    if(move_list.count() == 0) {
        throw std::invalid_argument("MinimaxAI::compute_move() - no legal moves!");
    }

    m_stats.reset();
    m_tt.new_search_iteration();
    m_deadline = std::chrono::steady_clock::now()
               + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(m_time_limit_seconds));
    m_stop_search = false;
    m_nodes_visited = 0;

    Move best_move = 0;
    int32_t prev_score = 0;
    
    int target_depth = 1;
    for (; target_depth <= m_max_depth; ++target_depth) {
        int32_t score;
        Move move;

        if (m_aspiration_window_enabled) {
            // Aspiration window search
            uint32_t nodes_before = m_stats.alpha_beta_nodes;
            int32_t alpha = std::max(-INF_SCORE, prev_score - m_aspiration_window);
            int32_t beta  = std::min(INF_SCORE,  prev_score + m_aspiration_window);
            std::tie(score, move) = _root_search(alpha, beta, target_depth, best_move);

            if (m_stop_search)
                break;

            // failed low or high, do a full window search
            if (score <= alpha || score >= beta) {
                ++m_stats.aspiration_misses;
                m_stats.aspiration_miss_nodes += m_stats.alpha_beta_nodes - nodes_before;
                std::tie(score, move) = _root_search(-INF_SCORE, INF_SCORE, target_depth, best_move);
            }
        }
        else {
            // Normal search
            std::tie(score, move) = _root_search(-INF_SCORE, INF_SCORE, target_depth, best_move);
        }

        if (m_stop_search)
            break;

        prev_score = score;
        best_move = move;
    }

    m_stats.depth = target_depth - 1;
    m_stats.eval = prev_score;
    m_stats.time_seconds = m_time_limit_seconds + std::chrono::duration<double>(m_deadline - std::chrono::steady_clock::now()).count();

    if (m_enable_debug_output)
        m_stats.print();

    return MoveEncoding::to_uci(best_move);
}

std::pair<int32_t, Move> MinimaxAI::_root_search(int32_t alpha, int32_t beta, int32_t search_depth, Move previous_best) {
    ++m_stats.alpha_beta_nodes;

    Color side = m_search_position.get_position().get_side_to_move();
    Move best_move = 0;

    MoveList move_list;
    move_list.generate<GenerateType::Legal>(m_search_position.get_position());
    _order_moves(move_list, previous_best);

    for (const Move& move : move_list) {
        m_search_position.make_move(move);
        int32_t score = -_alpha_beta(-beta, -alpha, search_depth - 1, 1);
        m_search_position.undo_move();

        if (m_stop_search) {
            return {NULL_SCORE, 0}; // timed out
        }

        if (score > alpha) {
            alpha = score;
            best_move = move;
        }
        if (alpha >= beta) {
            // refutation move found, fail-high node
            return {alpha, best_move};
        }
    }

    return {alpha, best_move};
}

int32_t MinimaxAI::_alpha_beta(int32_t alpha, int32_t beta, int32_t depth, int32_t ply) {
    if (_timer_check())
        return NULL_SCORE;

    if (m_search_position.get_position().get_halfmove_clock() >= 100)
        return DRAW_SCORE; // fifty-move rule

    if (m_search_position.plies_since_irreversible_move() >= 4) {
        if(m_search_position.repetition_count() >= 3)
            return DRAW_SCORE; // threefold repetition
    }

    if (depth <= 0)
        return _quiescence(alpha, beta, ply);

    ++m_stats.alpha_beta_nodes;

    Color side = m_search_position.get_position().get_side_to_move();
    int32_t starting_alpha = alpha;

    uint64_t zobrist_key = m_search_position.get_position().get_zobrist_hash();
    const TTEntry* tt_entry = m_tt.find(zobrist_key);
    if (tt_entry != nullptr) ++m_stats.tt_raw_hits;
    if (tt_entry != nullptr && tt_entry->depth >= depth) {
        ++m_stats.tt_usable_hits;
        // use stored entry (adjusted mate-distance for current ply)
        int32_t stored = adjust_score_from_tt(tt_entry->score, ply);
        if (tt_entry->bound == Bound::Exact) {
            ++m_stats.tt_cutoffs;
            return stored;
        } 
        else if (tt_entry->bound == Bound::Lower) {
            alpha = std::max(alpha, stored);
        }
        else if (tt_entry->bound == Bound::Upper) {
            beta = std::min(beta, stored);
        }
        if (alpha >= beta) {
            ++m_stats.tt_cutoffs;
            return stored;
        }
    }

    int32_t best_score = -INF_SCORE;
    Move best_move = 0;

    MoveList move_list;
    move_list.generate<GenerateType::Legal>(m_search_position.get_position());
    _order_moves(move_list, tt_entry ? tt_entry->best_move : 0);

    if (move_list.count() == 0) {
        if (m_search_position.get_position().in_check()) {
            // Checkmate with ply bonus to prefer faster mates
            int32_t store_score = -MATE_SCORE + ply;
            m_tt.store(zobrist_key, normalize_score_for_tt(store_score, ply), depth, Bound::Exact, 0);
            return store_score;
        } else {
            return DRAW_SCORE; // Stalemate
        }
    }

    for (const Move& move : move_list) {
        m_search_position.make_move(move);

        int32_t score = -_alpha_beta(-beta, -alpha, depth - 1, ply + 1);
        m_search_position.undo_move();

        if (m_stop_search) {
            return NULL_SCORE; // timed out
        }

        if (score > best_score) {
            best_score = score;
            best_move = move;

            if (score > alpha) {
                if (score < beta) {
                    // continue searching
                    alpha = score;
                }
                else {
                    // refutation move found, fail-high node
                    break;
                }
            }
        }
    }

    int32_t store_score = normalize_score_for_tt(best_score, ply);
    Bound bound = (best_score <= starting_alpha) ? Bound::Upper
                          : (best_score >= beta) ? Bound::Lower
                                                 : Bound::Exact;
    m_tt.store(zobrist_key, store_score, depth, bound, best_move);

    return best_score;
}

inline int32_t MinimaxAI::_quiescence(int32_t alpha, int32_t beta, int32_t ply) {
    ++m_stats.quiescence_nodes;
    if (_timer_check())
        return NULL_SCORE;

    MoveList move_list;
    int32_t best_score;
    
    if (!m_search_position.get_position().in_check()) {
        // Stand pat evaluation
        best_score = m_search_position.get_eval();
        if (best_score >= beta) return best_score;
        if (best_score > alpha) alpha = best_score;

        move_list.generate<GenerateType::Captures>(m_search_position.get_position());
        if (move_list.count() == 0) {
            // No captures and not in check, return static eval
            return alpha;
        }
    } 
    else {
        best_score = -INF_SCORE;
        move_list.generate<GenerateType::Evasions>(m_search_position.get_position());
        if (move_list.count() == 0) {
            // Checkmate with ply bonus to prefer faster mates
            return -MATE_SCORE + ply;
        }
    }

    // not probing TT in quiescence search, tested to be faster
    _order_moves(move_list, 0);

    if (move_list.count() == 0) 
        return alpha;

    for (const Move& move : move_list) {
        m_search_position.make_move(move);

        int32_t score = -_quiescence(-beta, -alpha, ply + 1);
        m_search_position.undo_move();

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                if (score < beta) {
                    alpha = score;
                }
                else {
                    // fail-high
                    break;
                }
            }
        }
    }

    return best_score;
}

inline void MinimaxAI::_order_moves(MoveList& move_list, const Move tt_move) const {
    struct Scored {
        Move move;
        int32_t score;
    };
    thread_local static std::array<Scored, MAX_MOVE_LIST_SIZE> scored;

    size_t n = move_list.count();
    for (size_t i = 0; i < n; i++) {
        scored[i].score = 0;
        scored[i].move = move_list[i];

        // TT best move top priority
        if (scored[i].move == tt_move) {
            scored[i].score += 10'000'000;
            continue;
        }

        // Captures, least valuable attacker capturing most valuable victim
        PieceType captured = m_search_position.get_position().to_capture(scored[i].move);
        if (captured != PieceType::None) {
            PieceType attacker = m_search_position.get_position().to_moved(scored[i].move);
            scored[i].score += 1'000'000 + m_search_position.material_value(captured) * 100 - m_search_position.material_value(attacker);
            continue;
        }

        // Promotions
        if (MoveEncoding::move_type(scored[i].move) == MoveType::Promotion) {
            PieceType promo = MoveEncoding::promo(scored[i].move);
            scored[i].score += 800'000 + m_search_position.material_value(promo) * 100;
            continue;
        }

        // Quiet moves left

        // Prefer non-pawn non-king moves
        PieceType mover = m_search_position.get_position().to_moved(scored[i].move);
        scored[i].score += (mover < PieceType::Pawn) ? 1'000 : 0;

        // Prefer moves that move towards center
        Square to = MoveEncoding::to_sq(scored[i].move);
        int center_dist = std::abs(3 - file_of(to)) + std::abs(3 - rank_of(to));
        scored[i].score += (10 - center_dist);
    }

    if (n <= 32) {
        // insertion sort (descending)
        for (size_t i = 1; i < n; ++i) {
            Scored item = scored[i];
            ssize_t j = (ssize_t)i - 1;
            while (j >= 0 && scored[j].score < item.score) {
                move_list[j + 1] = move_list[j];
                scored[j + 1] = scored[j];
                --j;
            }
            move_list[j + 1] = item.move;
            scored[j + 1] = item;
        }
    }
    else {
        // normal sort (descending)
        std::stable_sort(scored.begin(), scored.begin() + n, [&](Scored a, Scored b) {
            return a.score > b.score;
        });
        for (size_t i = 0; i < n; ++i)
            move_list[i] = scored[i].move;
    }
}

inline bool MinimaxAI::_timer_check() {
    constexpr uint32_t mask = (1<<10) - 1; // every 1024 nodes
    if ((++m_nodes_visited & mask) == 0) {
        if (std::chrono::steady_clock::now() >= m_deadline || _stop_requested()) {
            m_stop_search = true;
        }
    }
    return m_stop_search;
}
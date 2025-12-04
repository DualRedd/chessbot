#include "ai/ai_minimax.hpp"
#include<iostream> // DEBUG

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
        {"aspiration_window", "Aspiration window (centipawns)", FieldType::Int, 50}
    };

    AIRegistry::registerAI("Minimax", cfg, createMinimaxAI);
}

MinimaxAI::MinimaxAI(const std::vector<ConfigField>& cfg)
  : m_search_position(),
    m_tt(30ULL),
    m_time_limit_seconds(get_config_field_value<double>(cfg, "time_limit")),
    m_aspiration_window(get_config_field_value<int>(cfg, "aspiration_window"))
{}

void MinimaxAI::_set_board(const FEN& fen) {
    m_search_position.set_board(fen);
}

void MinimaxAI::_apply_move(const UCI& uci_move) {
    MoveList move_list;
    Move move = m_search_position.get_position().move_from_uci(uci_move);
    move_list.generate_legal(m_search_position.get_position());
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
    move_list.generate_legal(m_search_position.get_position());
    if(move_list.count() == 0) {
        throw std::invalid_argument("MinimaxAI::compute_move() - no legal moves!");
    }

    m_tt.new_search_iteration();
    m_deadline = std::chrono::steady_clock::now()
               + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(m_time_limit_seconds));
    m_stop_search = false;
    m_nodes_visited = 0;

    uint64_t zobrist_key = m_search_position.get_position().get_zobrist_hash();
    Move best_move = 0;
    int32_t prev_score = 0;
    
    int target_depth = 1;
    for (; target_depth <= 999; ++target_depth) {
        int32_t alpha = std::max(-INF_SCORE, prev_score - m_aspiration_window);
        int32_t beta  = std::min(INF_SCORE,  prev_score + m_aspiration_window);

        //auto [score, move] = _root_search(-INF_SCORE, INF_SCORE, target_depth);
        // Aspiration window search
        auto [score, move] = _root_search(alpha, beta, target_depth);

        if (_timer_check())
            break;

        // failed low or high, do a full window search
        if (score <= alpha || score >= beta) {
            std::tie(score, move) = _root_search(-INF_SCORE, INF_SCORE, target_depth);
        }

        if (_timer_check())
            break;

        int32_t store_score = normalize_score_for_tt(score, 0);
        m_tt.store(zobrist_key, store_score, target_depth, Bound::Exact, move);

        prev_score = score;
        best_move = move;

        std::cout << "MinimaxAI: depth " << target_depth
                  << ", best move " << MoveEncoding::to_uci(best_move)
                  << ", score " << score
                  << ", total nodes " << m_nodes_visited << std::endl;
    }
    
    return MoveEncoding::to_uci(best_move);
}

std::pair<int32_t, Move> MinimaxAI::_root_search(int32_t alpha, int32_t beta, int32_t search_depth) {
    uint64_t zobrist_key = m_search_position.get_position().get_zobrist_hash();
    Color side = m_search_position.get_position().get_side_to_move();
    Move best_move = 0;

    MoveList move_list;
    move_list.generate_legal(m_search_position.get_position());
    _order_moves(move_list, m_tt.find(zobrist_key));
    
    for (const Move& move : move_list) {
        m_search_position.make_move(move); 
        assert(!m_search_position.get_position().in_check(side));

        int32_t score = -_alpha_beta(-beta, -alpha, search_depth - 1, 1);
        if (score > alpha) {
            alpha = score;
            best_move = move;
        }
        m_search_position.undo_move();
    }

    return {alpha, best_move};
}

int32_t MinimaxAI::_alpha_beta(int32_t alpha, int32_t beta, int depth, int ply) {
    if (_timer_check())
        return DRAW_SCORE;

    if (m_search_position.get_position().get_halfmove_clock() >= 100)
        return DRAW_SCORE; // fifty-move rule

    if (m_search_position.plies_since_irreversible_move() >= 4) {
        if(m_search_position.repetition_count() >= 3)
            return DRAW_SCORE; // threefold repetition
    }

    if (depth == 0)
        return _quiescence(alpha, beta, ply);
    
    uint64_t zobrist_key = m_search_position.get_position().get_zobrist_hash();
    const TTEntry* tt_entry = m_tt.find(zobrist_key);
    
    if (tt_entry != nullptr && tt_entry->depth >= depth) {
        // use stored entry (adjusted mate-distance for current ply)
        int32_t stored = adjust_score_from_tt(tt_entry->score, ply);
        Bound bound = static_cast<Bound>(tt_entry->bound);
        if (bound == Bound::Exact) return stored;
        if (bound == Bound::Lower) alpha = std::max(alpha, stored);
        else if (bound == Bound::Upper) beta = std::min(beta, stored);
        if (alpha >= beta) return stored;
    }

    Color side = m_search_position.get_position().get_side_to_move();
    int32_t starting_alpha = alpha;
    Move best_move;

    MoveList move_list;
    move_list.generate_legal(m_search_position.get_position());
    _order_moves(move_list, tt_entry);

    if (move_list.count() == 0) {
        if (m_search_position.get_position().in_check(side)) {
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
        assert(!m_search_position.get_position().in_check(side));

        int32_t score = -_alpha_beta(-beta, -alpha, depth - 1, ply + 1);
        if (score > alpha) {
            // best move found so far
            alpha = score;
            best_move = move;
        }
        if (alpha >= beta) {
            // refutation move found, fail-high node
            m_search_position.undo_move();
            int32_t store_score = normalize_score_for_tt(alpha, ply);
            m_tt.store(zobrist_key, store_score, depth, Bound::Lower, best_move);
            return beta;
        }
        m_search_position.undo_move();
    }

    int32_t store_score = normalize_score_for_tt(alpha, ply);
    Bound bound = (alpha > starting_alpha) ? Bound::Exact : Bound::Upper;
    m_tt.store(zobrist_key, store_score, depth, bound, best_move);

    return alpha;
}

inline int32_t MinimaxAI::_quiescence(int32_t alpha, int32_t beta, int ply) {
    if (_timer_check())
        return DRAW_SCORE;

    // Stand pat evaluation
    int32_t best_score = m_search_position.get_eval();
    if (best_score >= beta) return best_score;
    if (best_score > alpha) alpha = best_score;

    uint64_t zobrist_key = m_search_position.get_position().get_zobrist_hash();
    MoveList move_list;
    move_list.generate_legal(m_search_position.get_position());
    _order_moves(move_list, m_tt.find(zobrist_key));

    if (move_list.count() == 0) 
        return alpha;

    for (const Move& move : move_list) {
        // Only consider captures in quiescence search
        PieceType captured = m_search_position.get_position().to_capture(move);
        if (captured == PieceType::None)
            continue;

        m_search_position.make_move(move);
        int32_t score = -_quiescence(-beta, -alpha, ply + 1);
        m_search_position.undo_move();
        if (score >= beta) return score;
        if (score > best_score) best_score = score;
        if (score > alpha) alpha = score;
    }

    return best_score;
}

inline void MinimaxAI::_order_moves(MoveList& move_list, const TTEntry* tt_entry) const {
    thread_local static std::array<int32_t, MoveList::MAX_SIZE> scores;
    Move tt_best = tt_entry ? tt_entry->best_move : 0;

    for (size_t i = 0; i < move_list.count(); i++) {
        scores[i] = 0;
        Move& move = move_list[i];
        MoveType move_type = MoveEncoding::move_type(move);

        // TT best move top priority
        if (move == tt_best) {
            scores[i] += 10'000'000;
        }

        // Captures, least valuable attacker capturing most valuable victim
        PieceType captured = m_search_position.get_position().to_capture(move);
        if (captured != PieceType::None) {
            PieceType attacker = m_search_position.get_position().to_moved(move);
            scores[i] += 1'000'000 + m_search_position.material_value(captured) * 100 - m_search_position.material_value(attacker);
        }

        // Promotions
        if (move_type == MoveType::Promotion) {
            PieceType promo = MoveEncoding::promo(move);
            scores[i] += 800'000 + m_search_position.material_value(promo) * 100;
        }

        // Prefer quiet moves that move towards center
        Square to = MoveEncoding::to_sq(move);
        int center_dist = std::abs(3 - file_of(to)) + std::abs(3 - rank_of(to));
        scores[i] += (10 - center_dist);
    }

    // insertion sort (descending)
    for (size_t i = 1; i < move_list.count(); ++i) {
        Move move = move_list[i];
        int32_t score = scores[i];
        ssize_t j = (ssize_t)i - 1;
        while (j >= 0 && scores[j] < score) {
            move_list[j + 1] = move_list[j];
            scores[j + 1] = scores[j];
            --j;
        }
        move_list[j + 1] = move;
        scores[j + 1] = score;
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
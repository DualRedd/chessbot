#include "ai/ai_minimax.hpp"

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
        return std::make_unique<MinimaxAI>();
    };

    AIRegistry::registerAI("Minimax", {}, createMinimaxAI);
}

MinimaxAI::MinimaxAI()
  : m_tt(30ULL) {}

void MinimaxAI::_set_board(const FEN& fen) {
    m_position.set_board(fen);
}

void MinimaxAI::_apply_move(const UCI& uci_move) {
    Move move = m_position.get_board().move_from_uci(uci_move);
    auto legal_moves = m_position.get_board().generate_legal_moves();
    if (std::find(legal_moves.begin(), legal_moves.end(), move) == legal_moves.end()) {
        throw std::invalid_argument("MinimaxAI::apply_move() - illegal move!");
    }
    m_position.make_move(move);
}

void MinimaxAI::_undo_move() {
    if (!m_position.undo_move()) {
        throw std::invalid_argument("MinimaxAI::undo_move() - no previous move!");
    }
}

UCI MinimaxAI::_compute_move() {
    m_tt.new_search_iteration();
    uint64_t zobrist_key = m_position.get_board().get_zobrist_hash();
    PlayerColor side = m_position.get_board().get_side_to_move();
    int32_t alpha = -INF_SCORE;
    int32_t beta = INF_SCORE;
    Move best_move = 0;

    int legal_move_count = 0;
    auto moves = m_position.get_board().generate_pseudo_legal_moves();
    _order_moves(moves, m_tt.find(zobrist_key));
    
    for (const Move& move : moves) {
        m_position.make_move(move); 
        if (!m_position.get_board().in_check(side)) {
            legal_move_count++;
            int32_t score = -_alpha_beta(-beta, -alpha, m_search_depth - 1, 1);
            if (score > alpha) {
                alpha = score;
                best_move = move;
            }
        }
        m_position.undo_move();
    }

    if (legal_move_count == 0) {
        throw std::invalid_argument("MinimaxAI::compute_move() - no legal moves!");
    }

    int32_t store_score = normalize_score_for_tt(alpha, 0);
    m_tt.store(zobrist_key, store_score, m_search_depth, Bound::Exact, best_move);
    return MoveEncoding::to_uci(best_move);
}

int32_t MinimaxAI::_alpha_beta(int32_t alpha, int32_t beta, int depth, int ply) {
    if (m_position.get_board().get_halfmove_clock() >= 100)
        return DRAW_SCORE; // fifty-move rule

    if (m_position.plies_since_irreversible_move() >= 4) {
        if(m_position.repetition_count() >= 3) {
            return DRAW_SCORE; // threefold repetition
        }
    }

    if (depth == 0)
        return m_position.get_eval();
    
    uint64_t zobrist_key = m_position.get_board().get_zobrist_hash();
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

    PlayerColor side = m_position.get_board().get_side_to_move();
    int32_t starting_alpha = alpha;
    Move best_move;

    int legal_move_count = 0;
    auto moves = m_position.get_board().generate_pseudo_legal_moves();
    _order_moves(moves, tt_entry);

    for (const Move& move : moves) {
        m_position.make_move(move);
        if (!m_position.get_board().in_check(side)) {
            legal_move_count++;
            int32_t score = -_alpha_beta(-beta, -alpha, depth - 1, ply + 1);
            if (score > alpha) {
                // best move found so far
                alpha = score;
                best_move = move;
            }
            if (alpha >= beta) {
                // refutation move found, fail-high node
                m_position.undo_move();
                int32_t store_score = normalize_score_for_tt(alpha, ply);
                m_tt.store(zobrist_key, store_score, depth, Bound::Lower, best_move);
                return alpha;
            }
        }
        m_position.undo_move();
    }

    if (legal_move_count == 0) {
        if (m_position.get_board().in_check(side)) {
            // Checkmate with ply bonus to prefer faster mates
            int32_t store_score = -MATE_SCORE + ply;
            m_tt.store(zobrist_key, normalize_score_for_tt(store_score, ply), depth, Bound::Exact, 0);
            return store_score;
        } else {
            return DRAW_SCORE; // Stalemate
        }
    }

    int32_t store_score = normalize_score_for_tt(alpha, ply);
    if (alpha > starting_alpha) {
        m_tt.store(zobrist_key, store_score, depth, Bound::Exact, best_move);
    } else {
        m_tt.store(zobrist_key, store_score, depth, Bound::Upper, best_move);
    }

    return alpha;
}

inline void MinimaxAI::_order_moves(std::vector<Move>& moves, const TTEntry* tt_entry) const {
    Move tt_best = tt_entry ? static_cast<Move>(tt_entry->best_move) : 0;
    
    std::vector<std::pair<int32_t, Move>> scored;
    scored.reserve(moves.size());

    for (const Move &mv : moves) {
        int32_t score = 0;

        // TT best move gets top priority
        if (mv == tt_best) {
            score += 10'000'000;
        }

        // Captures, least valuable attacker capturing most valuable victim
        PieceType victim = MoveEncoding::capture(mv);
        if (victim != PieceType::None) {
            PieceType attacker = MoveEncoding::piece(mv);
            score += 1'000'000 + m_position.material_value(victim) * 100 - m_position.material_value(attacker);
        }

        // Promotions
        PieceType promo = MoveEncoding::promo(mv);
        if (promo != PieceType::None) {
            score += 800'000 + m_position.material_value(promo) * 100;
        }

        // Prefer quiet moves that move towards center
        int to = MoveEncoding::to_sq(mv);
        int rank = to / 8;
        int file = to % 8;
        int center_dist = std::abs(3 - file) + std::abs(3 - rank);
        score += (10 - center_dist);

        scored.push_back({score, mv});
    }

    std::sort(scored.begin(), scored.end(), [](const auto &a, const auto &b){ return a.first > b.first; });

    for (size_t i = 0; i < scored.size(); ++i) {
        moves[i] = scored[i].second;
    }
}

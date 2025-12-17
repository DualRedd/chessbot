#include "engine/minimax_engine.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "engine/move_picker.hpp"
#include "engine/value_tables.hpp"
#include "engine/see.hpp"

static constexpr int32_t NO_SCORE = 111'111'111;
static constexpr int32_t INF_SCORE = 100'000'000;
static constexpr int32_t MATE_SCORE = 1'000'000;
static constexpr int32_t DRAW_SCORE = 0;

// time in milliseconds since epoch
static inline int32_t now_milliseconds() {
    using namespace std::chrono;
    return static_cast<int32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// Adjust a score to be stored in the TT (mate distance encoding)
static inline int32_t normalize_score_for_tt(int32_t score, int ply) {
    if (score > MATE_SCORE - 1000)
        return score + ply;
    if (score < -MATE_SCORE + 1000)
        return score - ply;
    return score;
}

// Adjust a stored TT score back to the current ply (mate distance decoding)
static inline int32_t adjust_score_from_tt(int32_t stored_score, int ply) {
    if (stored_score > MATE_SCORE - 1000)
        return stored_score - ply;
    if (stored_score < -MATE_SCORE + 1000)
        return stored_score + ply;
    return stored_score;
}

// check side to move has a pawn on rank 7 (white) or rank 2 (black)
static inline bool promotion_possible(const Position& pos) {
    const Color stm = pos.get_side_to_move();
    const Bitboard rank_7 = stm == Color::White? MASK_RANK[6] : MASK_RANK[1];
    return (pos.get_pieces(stm, PieceType::Pawn) & rank_7) != 0ULL;
}

static inline bool has_non_pawn_material(const Position& pos) {
    Bitboard pieces = pos.get_pieces(PieceType::All) & ~(
        pos.get_pieces(PieceType::Pawn) |
        pos.get_pieces(PieceType::King)
    );
    return pieces != 0ULL;
}

// A score is a win if it is a mate score for the side to move
static inline bool is_win(int32_t score) {
    return score < INF_SCORE && score > MATE_SCORE - 1000;
}
// A score is a loss if it is a mate score for the opponent
static inline bool is_loss(int32_t score) {
    return score > -INF_SCORE && score < -MATE_SCORE + 1000;
}
// A score is decisive if it is a mate score (win or loss)
static inline bool is_decisive(int32_t score) {
    return is_win(score) || is_loss(score);
}
// Convert a mate score to number of moves until mate, zero if not a mate score
static inline int32_t to_mate_distance(int32_t score) {
    if (is_win(score)) {
        return (MATE_SCORE - score + 1) / 2;
    } else if (is_loss(score)) {
        return -(MATE_SCORE + score) / 2;
    } else {
        return 0; // not a mate score
    }
}
// create a score to represent mate in a given number of plies
static inline int32_t mated_in(int32_t ply) {
    return -MATE_SCORE + ply;
}


void registerMinimaxAI() {
    auto createMinimaxAI = [](const std::vector<ConfigField>& cfg) {
        return std::make_unique<MinimaxAI>(cfg);
    };

    std::vector<ConfigField> cfg = {
        {"enable_uci_output", "Enable console UCI output", FieldType::Bool, false},
        {"time_limit", "Thinking time (s)", FieldType::Double, 5.0},
        {"max_depth", "Maximum search depth", FieldType::Int, 99},
        {"tt_size_megabytes", "Transposition table size (MB)", FieldType::Int, 256},
    };

    AIRegistry::registerAI("Minimax", cfg, createMinimaxAI);
}

MinimaxAI::MinimaxAI(const std::vector<ConfigField>& cfg)
  : m_max_depth(get_config_field_value<int>(cfg, "max_depth")),
    m_time_limit_seconds(get_config_field_value<double>(cfg, "time_limit")),
    m_tt_size_megabytes(get_config_field_value<int>(cfg, "tt_size_megabytes")),
    m_spos(),
    m_tt(m_tt_size_megabytes),
    m_enable_uci_output(get_config_field_value<bool>(cfg, "enable_uci_output"))
{}

MinimaxAI::MinimaxAI(const int32_t max_depth,
                    const double time_limit_seconds,
                    const size_t tt_size_megabytes,
                    const bool enable_uci_output) 
  : m_max_depth(max_depth),
    m_time_limit_seconds(time_limit_seconds),
    m_tt_size_megabytes(tt_size_megabytes),
    m_spos(),
    m_tt(m_tt_size_megabytes),
    m_enable_uci_output(enable_uci_output)
{}

void MinimaxAI::set_time_limit_seconds(double secs) {
    m_time_limit_seconds = secs < 0.0 ? 1e6 : secs;
}
void MinimaxAI::set_max_depth(int depth) {
    m_max_depth = depth < 0 ? 9999 : depth;
}
void MinimaxAI::set_max_nodes(int64_t nodes) {
    m_max_nodes = nodes < 0 ? std::numeric_limits<int64_t>::max() : nodes;
}
void MinimaxAI::clear_transposition_table() {
    m_tt.clear();
}

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
    m_spos.set_board(fen);
}

void MinimaxAI::_apply_move(const UCI& uci_move) {
    MoveList move_list;
    Move move = m_spos.get_position().move_from_uci(uci_move);
    move_list.generate<GenerateType::Legal>(m_spos.get_position());
    if (std::find(move_list.begin(), move_list.end(), move) == move_list.end())
        throw std::invalid_argument("MinimaxAI::apply_move() - illegal move!");
    m_spos.make_move(move);
}

void MinimaxAI::_undo_move() {
    if (!m_spos.undo_move())
        throw std::invalid_argument("MinimaxAI::undo_move() - no previous move!");
}

std::pair<int, UCI> MinimaxAI::find_mate() {
    UCI best_move = compute_move();
    return {to_mate_distance(m_stats.eval), best_move};
}

UCI MinimaxAI::_compute_move() {
    MoveList move_list;
    move_list.generate<GenerateType::Legal>(m_spos.get_position());
    if(move_list.count() == 0)
        throw std::invalid_argument("MinimaxAI::compute_move() - no legal moves!");

    // Prepare next search
    m_stats.reset();
    m_tt.new_search_iteration();
    m_killer_history.reset();

    m_start_time = now_milliseconds();
    m_deadline = m_start_time + static_cast<int32_t>(m_time_limit_seconds * 1000.0);
    m_stop_search = false;
    m_nodes_visited = 0;
    m_seldepth = 0;
    
    Move best_move = NO_MOVE;
    int32_t best_score = -INF_SCORE;
    
    // Iterative deepening loop
    int target_depth = 1;
    for (; target_depth <= m_max_depth; ++target_depth) {
        if (m_enable_uci_output)
            std::cout << "info depth " << target_depth << "\n" << std::flush;

        // Normal search
        m_root_best_move = NO_MOVE;
        m_root_best_score = -INF_SCORE;
        _alpha_beta<NodeType::Root>(-INF_SCORE, INF_SCORE, target_depth, 0);

        if (m_stop_search) {
            // Timed out! Can still use partial result if root managed to find a better move.
            // But don't update best move if either score is decisive, because the score is likely inaccurate for mate scores.
            if (m_root_best_score > best_score && !is_decisive(best_score)) {
                best_move = m_root_best_move;
                best_score = m_root_best_score;
            }
            break;
        }

        best_move = m_root_best_move;
        best_score = m_root_best_score;

        if (m_enable_uci_output) {
            int32_t time_elapsed = now_milliseconds() - m_start_time;
            std::cout << "info depth " << target_depth << " seldepth " << m_seldepth << " score ";
            if (is_decisive(m_root_best_score)) std::cout << "mate " << to_mate_distance(m_root_best_score);
            else std::cout << "cp " << m_root_best_score;
            std::cout << " nodes " << m_stats.alpha_beta_nodes
                    << " nps " << (time_elapsed == 0 ? "inf" : std::to_string(static_cast<int>(static_cast<double>(m_stats.alpha_beta_nodes + m_stats.quiescence_nodes) / time_elapsed * 1000.0)))
                    << " time " << time_elapsed << " pv ";
            int pv_length = 0;
            for (size_t i = 0; i < target_depth; ++i) {
                const TTEntry* tt_entry = m_tt.find(m_spos.get_position().get_key());
                if (!tt_entry || tt_entry->best_move == NO_MOVE)
                    break;
                std::cout << MoveEncoding::to_uci(tt_entry->best_move) << " ";
                m_spos.make_move(tt_entry->best_move);
                ++pv_length;
            }
            std::cout << "\n" << std::flush;
            for (size_t i = 0; i < pv_length; ++i)
                m_spos.undo_move();
        }
    }

    if (best_move == NO_MOVE) {
        if (target_depth != 1)
            throw std::runtime_error("MinimaxAI::_compute_move() - missing move result!");

        // no best move found (timeout on first iteration), choose one legal move
        if (m_enable_uci_output)
            std::cout << "info search stopped during first iteration!\n" << std::flush;
        best_move = move_list[0];
    }

    m_stats.depth = target_depth - 1;
    m_stats.eval = best_score;
    m_stats.time_seconds = static_cast<double>(now_milliseconds() - m_start_time) / 1000.0;

    UCI uci_best_move = MoveEncoding::to_uci(best_move);
    if (m_enable_uci_output)
        std::cout << "bestmove " << uci_best_move << "\n" << std::flush;

    return uci_best_move;
}


template<NodeType node_type>
int32_t MinimaxAI::_alpha_beta(int32_t alpha, int32_t beta, const int32_t depth, const int32_t ply, const int32_t prior_reductions) {
    constexpr bool is_root = (node_type == NodeType::Root);
    constexpr bool is_pv = (node_type == NodeType::PV || is_root);

    if (_stop_check())
        return NO_SCORE;

    // Track deepest selective pv line searched
    if (is_pv && m_seldepth < ply)
        m_seldepth = ply;

    // Check for draws
    if (m_spos.get_position().get_halfmove_clock() >= 100)
        return DRAW_SCORE; // fifty-move rule

    if (m_spos.plies_since_irreversible_move() >= 4) {
        if(m_spos.repetition_count() >= 3)
            return DRAW_SCORE; // threefold repetition
    }

    // Go into quiescence search at leaf nodes
    if (depth <= 0)
        return _quiescence(alpha, beta, ply);

    ++m_stats.alpha_beta_nodes;
    int32_t starting_alpha = alpha;

    // Probe transposition table
    uint64_t zobrist_key = m_spos.get_position().get_key();
    const TTEntry* tt_entry = m_tt.find(zobrist_key);
    if (tt_entry) ++m_stats.tt_raw_hits;

    // Check for cutoffs from TT entry
    if (!is_pv && tt_entry && tt_entry->depth >= depth) {
        ++m_stats.tt_usable_hits;
        int32_t stored = adjust_score_from_tt(tt_entry->score, ply);

        if (tt_entry->bound == Bound::Lower) {
            alpha = std::max(alpha, stored);
        }
        else if (tt_entry->bound == Bound::Upper) {
            beta = std::min(beta, stored);
        }
        if (tt_entry->bound == Bound::Exact || alpha >= beta) {
            ++m_stats.tt_cutoffs;
            return stored;
        }
    }

    int32_t static_eval = m_spos.get_eval();
    const bool in_check = m_spos.get_position().in_check();

    // Null move pruning
    // If the static eval is high enough, do a null move (pass the turn) and see if we still get a a beta cutoff (with much reduced depth).
    // The idea being that if we can get a cutoff without moving, we can also get one when moving.
    // The exception being zugzwangs. Which is why we check for non-pawn material, as zugzwangs are more common in pawn endgames.
    // The is_null_window and previous_was_capture conditions are some ideas, that can improve tactical stability.
    bool is_null_window = !is_pv && alpha == beta - 1;
    bool previous_was_capture = !is_root && m_spos.get_position().get_last_move_capture() != Piece::None;
    if (!is_root && (is_null_window || !previous_was_capture) && !in_check && depth >= 3 && has_non_pawn_material(m_spos.get_position()) && static_eval >= 53 + beta) {
        m_spos.make_null_move();
        const int32_t R = 3 + (depth >= 8); // reduction
        int32_t score = -_alpha_beta<NodeType::NonPV>(-beta - 1, -beta, depth - 1 - R, ply + 1, prior_reductions);
        m_spos.undo_null_move();

        if (m_stop_search)
            return NO_SCORE; // timed out

        if (score >= beta)
            return score; 
    }
    
    Move best_move = NO_MOVE;
    int32_t best_score = -INF_SCORE;

    MovePicker move_picker(m_spos.get_position(), ply, tt_entry ? tt_entry->best_move : NO_MOVE,
                            &m_killer_history, &m_move_history);
    int move_count = 0;

    for (Move move = move_picker.next(); move != NO_MOVE; move = move_picker.next()) {
        ++move_count;
        if (is_root && m_enable_uci_output && now_milliseconds() - m_start_time >= 5000) {
            std::cout << "info depth " << depth << " currmove " << MoveEncoding::to_uci(move)
                        << " currmovenumber " << move_count << "\n" << std::flush;
        }

        int32_t new_depth = depth - 1;
        const bool gives_check = m_spos.get_position().gives_check(move);
        const bool is_capture = m_spos.get_position().to_capture(move) != PieceType::None;

        // Futility pruning
        // If the static eval is lower than alpha by a certain futility margin, we can just prune the move without searching it.
        // For tactical stability this is not done when in check, or when the move gives check or is a capture.
        if (!is_root && move_count > 1 && depth <= 3 && !in_check && !gives_check && !is_capture && !is_decisive(alpha)) {
            int32_t futility_value = static_eval + 48; // base margin
                                    + depth * 101 // depth based margin
                                    + m_move_history.get(m_spos.get_position(), move) / 16; // history bonus
            if (futility_value <= alpha) {
                if (best_score < futility_value && !is_decisive(best_score)) {
                    best_score = futility_value;
                    best_move = move;
                }
                continue;
            }
        }

        // Check extension for moves with good SEE values
        if (gives_check && static_exchange_evaluation(m_spos.get_position(), move, 0))
            new_depth += 1;

        // make move
        m_spos.make_move(move);
        int32_t score;

        // Principal Variation Search (PVS)
        // The PV line (expected principal variation) is searched with a full alpha-beta window,
        // while other moves are searched with a null window first to try to fail-low quickly.
        // In case a null window search fails high, we re-search with a full window, as it is the new PV.
        if (is_pv && move_count == 1) {
            // Search first move with full window
            score = -_alpha_beta<NodeType::PV>(-beta, -alpha, new_depth, ply + 1);
        }
        else {
            // Late move reduction
            // Reduce depth for moves that are not among the first few moves searched.
            // Assuming good move ordering the later moves should be worse,
            // so we can try to prove that with a reduced depth search first.
            int32_t reductions = 0;
            const bool lmr = move_count >= 3 && new_depth >= 3;
            if (lmr) {
                // Formula idea from https://www.chessprogramming.org/Late_Move_Reductions
                reductions += 1 + static_cast<int32_t>(floorf(logf(new_depth) * logf(move_count) / 3.2f));

                // limit total reductions to 3
                reductions = std::min(reductions, 3 - prior_reductions);
            }

            // Null window search with possible LMR
            score = -_alpha_beta<NodeType::NonPV>(-alpha - 1, -alpha, new_depth - reductions, ply + 1, prior_reductions + reductions);

            // Check if re-search is needed with LMR. Search first the full depth with a null window.
            // So assume for now the move failed high due to LMR rather than actually being a new PV.
            // A null window search is still very cheap.
            if (lmr && score > alpha && score < beta && !m_stop_search) {
                score = -_alpha_beta<NodeType::NonPV>(-alpha - 1, -alpha, new_depth, ply + 1, prior_reductions);
            }

            // Check if re-search is needed with null-window. If the full depth null window search failed high,
            // we need to re-search with a full window, as it is the new PV.
            if (score > alpha && score < beta && !m_stop_search) {
                score = -_alpha_beta<NodeType::PV>(-beta, -alpha, new_depth, ply + 1, prior_reductions);
            }
        }

        // undo move
        m_spos.undo_move();

        if (m_stop_search)
            return NO_SCORE; // timed out

        assert(score > -INF_SCORE && score < INF_SCORE);

        // Update best score/move and alpha based on the score
        if (score > best_score) {
            best_score = score;
            best_move = move;

            if constexpr (is_root) {
                // Check for new best move at root
                if (score > m_root_best_score) {
                    m_root_best_score = score;
                    m_root_best_move = best_move;
                }
            }

            if (score > alpha) {
                if (score < beta) {
                    // continue searching
                    alpha = score;
                }
                else {
                    // refutation move found, fail-high node
                    if (m_spos.get_position().to_capture(move) == PieceType::None) {
                        m_killer_history.store(move, ply);
                        m_move_history.update(m_spos.get_position(), move, depth * depth);
                    }
                    break;
                }
            }
        }
    }

    // Check for checkmate or stalemate
    if (move_count == 0) {
        best_score = in_check ? mated_in(ply) : DRAW_SCORE;
    }

    assert(best_score > -INF_SCORE && best_score < INF_SCORE);

    // Store the result in the transposition table
    int32_t store_score = normalize_score_for_tt(best_score, ply);
    Bound bound = (best_score <= starting_alpha) ? Bound::Upper
                            : (best_score >= beta) ? Bound::Lower
                                                    : Bound::Exact;
    m_tt.store(zobrist_key, store_score, depth, bound, best_move);

    return best_score;
}

inline int32_t MinimaxAI::_quiescence(int32_t alpha, int32_t beta, const int32_t ply) {
    ++m_stats.quiescence_nodes;

    if (_stop_check())
        return NO_SCORE;

    const int32_t static_eval = m_spos.get_eval();

    // Delta pruning before move generation
    // If even a big capture added to the static eval cannot raise alpha,
    // the position is simply bad, just return alpha.
    int32_t big_delta = PIECE_VALUES[+PieceType::Queen];
    if (promotion_possible(m_spos.get_position()))
        big_delta += PIECE_VALUES[+PieceType::Queen] - PIECE_VALUES[+PieceType::Pawn];
    if (static_eval + big_delta < alpha)
        return alpha;

    int32_t best_score = -INF_SCORE;

    const bool in_check = m_spos.get_position().in_check();
    if (!in_check) {
        // Standing pat evaluation
        // When not in check we can assume that at least one quiet move does not worsen the position.
        // This is based on the null move observation, and does not hold only in rare zugzwang positions.
        // Therefore, we can consider the static eval as the minimum score.
        if (static_eval >= beta)
            return static_eval;
        if (static_eval > alpha)
            alpha = static_eval;
        best_score = static_eval;
    }

    const int32_t material_phase = m_spos.material_phase();

    MovePicker move_picker(m_spos.get_position(), NO_MOVE);
    int move_count = 0;

    for (Move move = move_picker.next(); move != NO_MOVE; move = move_picker.next()) {
        ++move_count;

        // More delta pruning, disabled in endgame
        if (!in_check && material_phase > PHASE_LATE_ENDGAME) {
            int32_t delta_value =  static_eval + 150 + PIECE_VALUES[+m_spos.get_position().to_capture(move)];
            if (MoveEncoding::move_type(move) == MoveType::Promotion)
                delta_value += PIECE_VALUES[+PieceType::Queen] - PIECE_VALUES[+PieceType::Pawn];
            if (delta_value <= alpha) {
                if (best_score < delta_value && !is_decisive(best_score))
                    best_score = delta_value;
                continue;
            }
        }

        m_spos.make_move(move);
        int32_t score = -_quiescence(-beta, -alpha, ply + 1);
        m_spos.undo_move();

        if (m_stop_search)
            return NO_SCORE; // timed out

        assert(score > -INF_SCORE && score < INF_SCORE);

        // Update best score/move and alpha based on the score
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

    // Stalemates cannot be detected, as not all moves are generated
    // Checkmates can, because all evasions are generated
    if (in_check && move_count == 0) {
        best_score = mated_in(ply);
    }

    assert(best_score > -INF_SCORE && best_score < INF_SCORE);
    return best_score;
}

inline bool MinimaxAI::_stop_check() {
    constexpr int64_t mask = (1<<10) - 1; // every 1024 nodes
    if ((++m_nodes_visited & mask) == 0) {
        if (now_milliseconds() >= m_deadline
            || (m_max_nodes >= 0 && m_nodes_visited >= m_max_nodes)
            || _stop_requested()) {
            m_stop_search = true;
        }
    }
    return m_stop_search;
}

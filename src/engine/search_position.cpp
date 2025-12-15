#include "engine/search_position.hpp"

SearchPosition::SearchPosition() : m_position(), m_pawn_hash_table(32) {
    m_evals.reserve(200);
    m_zobrist_history.reserve(200);
    m_irreversible_move_plies.reserve(50);
}

void SearchPosition::set_board(const FEN& fen) {
    m_position.from_fen(fen);
    m_evals.clear();
    m_evals.emplace_back(_compute_full_eval());
    m_pawn_hash_table.clear();

    m_zobrist_history.clear();
    m_irreversible_move_plies.clear();
    m_irreversible_move_plies.push_back(0);
}

int32_t SearchPosition::get_eval() const {
    // Interpolate evaluation based on game phase
    const Eval& eval = m_evals.back();
    const int32_t phase = std::max(eval.phase - PHASE_MIN, 0);
    const int32_t mg_value = eval.mg_eval * phase;
    const int32_t eg_value = eval.eg_eval * (PHASE_WIDTH - phase);
    int32_t eval_value = (mg_value + eg_value) / PHASE_WIDTH;

    // Get pawn eval
    uint64_t pawn_key = m_position.get_pawn_key();
    const PawnTableEntry* entry = m_pawn_hash_table.find(pawn_key);
    if (entry) {
        eval_value += entry->eval;
    }
    else {
        int32_t pawn_eval = _eval_pawns();
        m_pawn_hash_table.store(pawn_key, pawn_eval);
        eval_value += pawn_eval;
    }

    return (m_position.get_side_to_move() == Color::White) ? eval_value : -eval_value;
}

int SearchPosition::repetition_count() const {
    int count = 1;
    uint64_t current_hash = m_position.get_key();
    for(size_t i = m_irreversible_move_plies.back(); i < m_zobrist_history.size(); i++) {
        count += m_zobrist_history[i] == current_hash;
    }
    return count;
}

int SearchPosition::plies_since_irreversible_move() const {
    return m_zobrist_history.size() - m_irreversible_move_plies.back();
}

int32_t SearchPosition::material_phase() const {
    int32_t material = 0;
    for (int type = 0; type < 6; ++type) {
        int32_t count = popcount(m_position.get_pieces(PieceType(type)));
        material += count * MATERIAL_WEIGHTS[type];
    }
    return std::min(material, PHASE_MAX);
}

void SearchPosition::make_move(Move move) {
    m_zobrist_history.push_back(m_position.get_key());

    const Eval& prev_eval = m_evals.back();
    m_evals.push_back(prev_eval);
    Eval& cur_eval = m_evals.back();

    MoveType move_type = MoveEncoding::move_type(move);

    if (move_type != MoveType::Normal) {
        // Slower recomputation for rarer move types
        m_position.make_move(move);
        cur_eval = _compute_full_eval();
    }
    else {
        const Color side = m_position.get_side_to_move();
        const Square from = MoveEncoding::from_sq(move);
        const Square to = MoveEncoding::to_sq(move);
        const PieceType piece_type = to_type(m_position.get_piece_at(from));
        const int32_t sign = (side == Color::White) ? 1 : -1;

        // Move piece
       cur_eval.mg_eval += sign * (_pst_value(piece_type, side, to, GamePhase::Middlegame)
                                - _pst_value(piece_type, side, from, GamePhase::Middlegame));
       cur_eval.eg_eval += sign * (_pst_value(piece_type, side, to, GamePhase::Endgame)
                                - _pst_value(piece_type, side, from, GamePhase::Endgame));

        // Handle capture
        PieceType captured = m_position.to_capture(move);
        if (captured != PieceType::None) {
            cur_eval.phase -= MATERIAL_WEIGHTS[+captured];

            cur_eval.mg_eval += sign * _material_value(captured);
            cur_eval.eg_eval += sign * _material_value(captured);

            const Color opp = opponent(side);
            cur_eval.mg_eval += sign * _pst_value(captured, opp, to, GamePhase::Middlegame);
            cur_eval.eg_eval += sign * _pst_value(captured, opp, to, GamePhase::Endgame);
        }

        m_position.make_move(move);
    }

    if (m_position.get_halfmove_clock() == 0) {
        // Halfmove clock reset
        m_irreversible_move_plies.push_back(m_zobrist_history.size());
    }
}

bool SearchPosition::undo_move() {
    if (m_evals.size() <= 1)
        return false;

    m_evals.pop_back();
    m_position.undo_move();

    if (m_irreversible_move_plies.back() == m_zobrist_history.size()) {
        m_irreversible_move_plies.pop_back();
    }
    m_zobrist_history.pop_back();

    return true;
}

void SearchPosition::make_null_move() {
    m_position.make_null_move();
}

void SearchPosition::undo_null_move() {
    m_position.undo_null_move();
}

const Position& SearchPosition::get_position() const {
    return m_position;
}

inline int32_t SearchPosition::_material_value(PieceType type) const {
    return PIECE_VALUES[+type];
}

inline int32_t SearchPosition::_pst_value(PieceType type, Color color, Square square, GamePhase stage) const {
    const Square idx = square_for_side(square, color);
    switch (type) {
        case PieceType::Pawn:   return PST_PAWN[+stage][+idx];
        case PieceType::Knight: return PST_KNIGHT[+stage][+idx];
        case PieceType::Bishop: return PST_BISHOP[+stage][+idx];
        case PieceType::Rook:   return PST_ROOK[+stage][+idx];
        case PieceType::Queen:  return PST_QUEEN[+stage][+idx];
        case PieceType::King:   return PST_KING[+stage][+idx];
        default: return 0;
    }
}

inline Eval SearchPosition::_compute_full_eval() {
    Eval eval = {0, 0, 0};
    eval.phase = material_phase();

    for (Square square = Square::A1; square <= Square::H8; ++square) {
        Piece piece = m_position.get_piece_at(square);
        if (piece == Piece::None) continue;

        const PieceType type = to_type(piece);
        const Color color = to_color(piece);
        const int32_t sign = (color == Color::White) ? 1 : -1;

        eval.mg_eval += sign * (_material_value(type) + _pst_value(type, color, square, GamePhase::Middlegame));
        eval.eg_eval += sign * (_material_value(type) + _pst_value(type, color, square, GamePhase::Endgame));
    }

    return eval;
}


template<Color color>
static inline int32_t eval_by_row(Bitboard pieces, const int32_t* value_table) {
    int32_t eval = 0;
    while (pieces) {
        Square sq = lsb(pieces);
        if constexpr (color == Color::White) {
            eval += value_table[rank_of(sq)];
        } else {
            eval += value_table[7 - rank_of(sq)];
        }
        pop_lsb(pieces);
    }
    return eval;
}

template<Color color>
static inline int32_t eval_by_file(Bitboard pieces, const int32_t* value_table) {
    int32_t eval = 0;
    while (pieces) {
        Square sq = lsb(pieces);
        if constexpr (color == Color::White) {
            eval += value_table[file_of(sq)];
        }
        else {
            eval += value_table[7 - file_of(sq)];
        }
        pop_lsb(pieces);
    }
    return eval;
}

inline int32_t SearchPosition::_eval_pawns() const {
    int32_t eval = 0;

    // doubled and tripled pawns
    const Bitboard w_pawns = m_position.get_pieces(Color::White, PieceType::Pawn);
    const Bitboard w_pawns_behind_own = w_pawns & front_spans<Color::Black>(w_pawns);
    const Bitboard w_pawns_ahead_own = w_pawns & front_spans<Color::White>(w_pawns);
    const Bitboard w_pawns_between_own = w_pawns_behind_own & w_pawns_ahead_own;
    eval += popcount(w_pawns_behind_own) * DOUBLED_PAWN_VALUE;
    eval += popcount(w_pawns_between_own) * TRIPLED_PAWN_VALUE;

    const Bitboard b_pawns = m_position.get_pieces(Color::Black, PieceType::Pawn);
    const Bitboard b_pawns_behind_own = b_pawns & front_spans<Color::White>(b_pawns);
    const Bitboard b_pawns_ahead_own = b_pawns & front_spans<Color::Black>(b_pawns);
    const Bitboard b_pawns_between_own = b_pawns_behind_own & b_pawns_ahead_own;
    eval -= popcount(b_pawns_behind_own) * DOUBLED_PAWN_VALUE;
    eval -= popcount(b_pawns_between_own) * TRIPLED_PAWN_VALUE;
    
    // isolated pawns
    const Bitboard w_no_neighbors_right = w_pawns & ~left_attack_file_fills(w_pawns);
    const Bitboard w_no_neighbors_left = w_pawns & ~right_attack_file_fills(w_pawns);
    const Bitboard w_isolated_pawns = w_no_neighbors_right & w_no_neighbors_left;
    eval += eval_by_file<Color::White>(w_isolated_pawns, ISOLATED_PAWN_VALUES);

    const Bitboard b_no_neighbors_right = b_pawns & ~left_attack_file_fills(b_pawns);
    const Bitboard b_no_neighbors_left = b_pawns & ~right_attack_file_fills(b_pawns);
    const Bitboard b_isolated_pawns = b_no_neighbors_right & b_no_neighbors_left;
    eval -= eval_by_file<Color::Black>(b_isolated_pawns, ISOLATED_PAWN_VALUES);

    // passed pawns
    const Bitboard b_all_front_spans = attack_front_spans<Color::Black>(b_pawns) | front_spans<Color::Black>(b_pawns);
    const Bitboard w_passed_pawns = w_pawns & ~b_all_front_spans & ~w_pawns_behind_own;
    eval += eval_by_row<Color::White>(w_passed_pawns, PASSED_PAWN_VALUES);

    const Bitboard w_all_front_spans = attack_front_spans<Color::White>(w_pawns) | front_spans<Color::White>(w_pawns);
    const Bitboard b_passed_pawns = b_pawns & ~w_all_front_spans & ~b_pawns_behind_own;
    eval -= eval_by_row<Color::Black>(b_passed_pawns, PASSED_PAWN_VALUES);

    // rammed pawns
    //const Bitboard w_rammed = shift_bb<pawn_dir(Color::Black)>(b_pawns) & w_pawns;
    //const Bitboard b_rammed = shift_bb<pawn_dir(Color::White)>(w_pawns) & b_pawns;

    // backward pawns (single pass)
    const Bitboard b_attacks = pawn_attacks<Color::Black>(b_pawns);
    const Bitboard b_controlled_stop_squares = b_attacks & ~attack_front_spans<Color::White>(w_pawns);
    const Bitboard w_backward_pawns = w_pawns & rear_spans<Color::White>(b_controlled_stop_squares);
    eval += popcount(w_backward_pawns) * BACKWARD_PAWN_VALUE;

    const Bitboard w_attacks = pawn_attacks<Color::White>(w_pawns);
    const Bitboard w_controlled_stop_squares = w_attacks & ~attack_front_spans<Color::Black>(b_pawns);
    const Bitboard b_backward_pawns = b_pawns & rear_spans<Color::Black>(w_controlled_stop_squares);
    eval -= popcount(b_backward_pawns) * BACKWARD_PAWN_VALUE;

    // defended pawns
    const Bitboard w_defended_pawns = w_pawns & w_attacks;
    eval += popcount(w_defended_pawns) * DEFENDED_PAWN_VALUE;

    const Bitboard b_defended_pawns = b_pawns & b_attacks;
    eval -= popcount(b_defended_pawns) * DEFENDED_PAWN_VALUE;

    return eval;
}

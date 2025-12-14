#include "engine/search_position.hpp"
#include<iostream>

SearchPosition::SearchPosition() : m_position() {
    m_zobrist_history.reserve(100);
    m_irreversible_move_plies.reserve(50);
}

void SearchPosition::set_board(const FEN& fen) {
    m_position.from_fen(fen);
    m_evals.clear();
    m_evals.emplace_back(_compute_full_eval());

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
    const int32_t eval_value = (mg_value + eg_value) / PHASE_WIDTH;

    return (m_position.get_side_to_move() == Color::White) ? eval_value : -eval_value;
}

int SearchPosition::repetition_count() const {
    int count = 1;
    uint64_t current_hash = m_position.get_zobrist_hash();
    for(size_t i = m_irreversible_move_plies.back(); i < m_zobrist_history.size(); i++) {
        count += m_zobrist_history[i] == current_hash;
    }
    return count;
}

int SearchPosition::plies_since_irreversible_move() const {
    return m_zobrist_history.size() - m_irreversible_move_plies.back();
}

void SearchPosition::make_move(Move move) {
    m_zobrist_history.push_back(m_position.get_zobrist_hash());

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

inline int32_t SearchPosition::_material_phase() const {
    int32_t material = 0;
    for (int type = 0; type < 6; ++type) {
        int32_t count = popcount(m_position.get_pieces(PieceType(type)));
        material += count * MATERIAL_WEIGHTS[type];
    }
    return std::min(material, PHASE_MAX);
}

inline Eval SearchPosition::_compute_full_eval() {
    Eval eval = {0, 0, 0};
    eval.phase = _material_phase();

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
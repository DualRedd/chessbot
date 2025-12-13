#include "engine/search_position.hpp"

#include "engine/value_tables.hpp"
#include <algorithm>

SearchPosition::SearchPosition() : m_position() {
    m_zobrist_history.reserve(100);
    m_irreversible_move_plies.reserve(50);
}

void SearchPosition::set_board(const FEN& fen) {
    m_position.from_fen(fen);
    m_eval = _compute_full_eval();

    m_zobrist_history.clear();
    m_irreversible_move_plies.clear();
    m_irreversible_move_plies.push_back(0);
}

int32_t SearchPosition::get_eval() const {
    return (m_position.get_side_to_move() == Color::White) ? m_eval : -m_eval;
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
    Color side = m_position.get_side_to_move();
    Color opp = opponent(side);
    Square from = MoveEncoding::from_sq(move);
    Square to = MoveEncoding::to_sq(move);
    MoveType move_type = MoveEncoding::move_type(move);
    PieceType piece_type = to_type(m_position.get_piece_at(from));

    int32_t delta = 0;

    // Move piece and handle promo
    if(move_type == MoveType::Promotion) {
        PieceType promo = MoveEncoding::promo(move);
        delta += _material_value(promo) - _material_value(piece_type);
        delta += _pst_value(promo, side, to) - _pst_value(piece_type, side, from);
    }
    else {
        delta += _pst_value(piece_type, side, to) - _pst_value(piece_type, side, from);
    }

    // Handle capture and en passant
    Square capture_square = to;
    if (move_type == MoveType::EnPassant) {
        capture_square = (side == Color::White) ? (to + Shift::Down) : (to + Shift::Up);
    }
    PieceType captured = to_type(m_position.get_piece_at(capture_square));
    if (captured != PieceType::None) {
        delta += _material_value(captured);
        delta += _pst_value(captured, opponent(side), capture_square);
    }

    // Handle castling
    if (move_type == MoveType::Castle) {
        Square rook_from = to > from ? from + 3 : from - 4;
        Square rook_to = to > from ? from + 1 : from - 1;
        delta += _pst_value(PieceType::Rook, side, rook_to) - _pst_value(PieceType::Rook, side, rook_from);
    }

    m_zobrist_history.push_back(m_position.get_zobrist_hash());
    if (piece_type == PieceType::Pawn || captured != PieceType::None) {
        // Halfmove clock reset
        m_irreversible_move_plies.push_back(m_zobrist_history.size());
    }

    m_eval_history.push_back(m_eval);
    m_eval += side == Color::White ? delta : -delta;
    m_position.make_move(move);
}

bool SearchPosition::undo_move() {
    if (m_eval_history.size() == 0)
        return false;

    m_eval = m_eval_history.back();
    m_eval_history.pop_back();
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

int32_t SearchPosition::_material_value(PieceType type) const {
    return PIECE_VALUES[+type];
}

int32_t SearchPosition::_pst_value(PieceType type, Color color, Square square) const {
    if (type == PieceType::None) return 0;

    Square idx = square_for_side(square, color);
    switch (type) {
        case PieceType::Pawn:   return PST_PAWN[+idx];
        case PieceType::Knight: return PST_KNIGHT[+idx];
        case PieceType::Bishop: return PST_BISHOP[+idx];
        case PieceType::Rook:   return PST_ROOK[+idx];
        case PieceType::Queen:  return PST_QUEEN[+idx];
        case PieceType::King:   return PST_KING[+idx];
        default: return 0;
    }
}

int32_t SearchPosition::_compute_full_eval() {
    int32_t eval = 0;

    for (Square square = Square::A1; square <= Square::H8; ++square) {
        Piece piece = m_position.get_piece_at(square);
        if (piece == Piece::None) continue;

        PieceType type = to_type(piece);
        Color color = to_color(piece);
        int32_t val = _material_value(type) + _pst_value(type, color, square);
        eval += (color == Color::White ? val : -val);
    }

    return eval;
}
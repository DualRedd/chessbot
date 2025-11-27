#include "ai/search_position.hpp"
#include "ai/pst.hpp"

#include <algorithm>

// Helper functions
constexpr inline PlayerColor opponent(PlayerColor side) { return side == PlayerColor::White ? PlayerColor::Black : PlayerColor::White; };
constexpr inline int square_for_side(int square, PlayerColor side) { return (side == PlayerColor::White) ? square : (63 - square); };

SearchPosition::SearchPosition() : m_board() {
    m_zobrist_history.reserve(100);
    m_irreversible_move_plies.reserve(50);
}

void SearchPosition::set_board(const FEN& fen) {
    m_board.set_from_fen(fen);
    m_eval = _compute_full_eval();

    m_zobrist_history.clear();
    m_irreversible_move_plies.clear();
    m_irreversible_move_plies.push_back(0);
}

int32_t SearchPosition::get_eval() const {
    return (m_board.get_side_to_move() == PlayerColor::White) ? m_eval : -m_eval;
}

int SearchPosition::repetition_count() const {
    int count = 1;
    uint64_t current_hash = m_board.get_zobrist_hash();
    for(size_t i = m_irreversible_move_plies.back(); i < m_zobrist_history.size(); i++) {
        count += m_zobrist_history[i] == current_hash;
    }
    return count;
}

int SearchPosition::plies_since_irreversible_move() const {
    return m_zobrist_history.size() - m_irreversible_move_plies.back();
}

void SearchPosition::make_move(Move move) {
    int from = MoveEncoding::from_sq(move);
    int to = MoveEncoding::to_sq(move);
    PieceType piece = MoveEncoding::piece(move);
    PieceType captured = MoveEncoding::capture(move);
    PieceType promo = MoveEncoding::promo(move);
    bool is_castle = MoveEncoding::is_castle(move);
    bool is_ep = MoveEncoding::is_en_passant(move);

    PlayerColor side = m_board.get_side_to_move();
    int32_t delta = 0;

    // Move piece and handle promo
    delta += _pst_value(piece, side, to) - _pst_value(piece, side, from);
    if (promo != PieceType::None) {
        delta += material_value(promo) - material_value(piece);
    }

    // Handle capture and en passant
    if (captured != PieceType::None) {
        int capture_square = is_ep ? (from & 0b111000 /* rank of from */) | (to & 0b000111 /* file of to */) : to;
        delta += material_value(captured);
        delta += _pst_value(captured, opponent(side), capture_square);
    }

    // Handle castling
    if (is_castle) {
        int rook_from = to > from ? from + 3 : from - 4;
        int rook_to = (to + from) >> 1;
        delta += _pst_value(PieceType::Rook, side, rook_to) - _pst_value(PieceType::Rook, side, rook_from);
    }

    m_zobrist_history.push_back(m_board.get_zobrist_hash());
    if (piece == PieceType::Pawn || captured != PieceType::None) {
        // Halfmove clock reset
        m_irreversible_move_plies.push_back(m_zobrist_history.size());
    }

    m_eval_history.push_back(m_eval);
    m_eval += side == PlayerColor::White ? delta : -delta;
    m_board.make_move(move);
}

bool SearchPosition::undo_move() {
    if (m_eval_history.size() == 0)
        return false;

    m_eval = m_eval_history.back();
    m_eval_history.pop_back();
    m_board.undo_move();

    if (m_irreversible_move_plies.back() == m_zobrist_history.size()) {
        m_irreversible_move_plies.pop_back();
    }
    m_zobrist_history.pop_back();

    return true;
}

const Board& SearchPosition::get_board() const {
    return m_board;
}

int32_t SearchPosition::material_value(PieceType type) const {
    switch(type) {
        case PieceType::Pawn:   return 100;
        case PieceType::Knight: return 320;
        case PieceType::Bishop: return 330;
        case PieceType::Rook:   return 500;
        case PieceType::Queen:  return 900;
        default: return 0;
    }
}

int32_t SearchPosition::_pst_value(PieceType type, PlayerColor color, int square) const {
    if (type == PieceType::None) return 0;

    int idx = square_for_side(square, color);
    switch (type) {
        case PieceType::Pawn:   return PST_PAWN[idx];
        case PieceType::Knight: return PST_KNIGHT[idx];
        case PieceType::Bishop: return PST_BISHOP[idx];
        case PieceType::Rook:   return PST_ROOK[idx];
        case PieceType::Queen:  return PST_QUEEN[idx];
        case PieceType::King:   return PST_KING[idx];
        default: return 0;
    }
}

int32_t SearchPosition::_compute_full_eval() {
    int32_t eval = 0;

    for (int square = 0; square < 64; square++) {
        Piece piece = m_board.get_piece_at(square);
        if (piece.type == PieceType::None) continue;

        int32_t val = material_value(piece.type) + _pst_value(piece.type, piece.color, square);
        eval += (piece.color == PlayerColor::White ? val : -val);
    }

    return eval;
}
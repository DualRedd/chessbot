#include "ai/search_position.hpp"
#include "ai/pst.hpp"

// Helper functions
constexpr inline PlayerColor opponent(PlayerColor side) { return side == PlayerColor::White ? PlayerColor::Black : PlayerColor::White; };
constexpr inline int square_for_side(int square, PlayerColor side) { return (side == PlayerColor::White) ? square : (63 - square); };


SearchPosition::SearchPosition() : m_board() {}

void SearchPosition::set_board(const FEN& fen) {
    m_board.set_from_fen(fen);
    m_eval = _compute_full_eval();
}

int SearchPosition::get_eval() const {
    return (m_board.get_side_to_move() == PlayerColor::White) ? m_eval : -m_eval;
}

PlayerColor SearchPosition::get_side_to_move() const {
    return m_board.get_side_to_move();
}

std::vector<Move> SearchPosition::generate_pseudo_legal_moves() const {
    return m_board.generate_pseudo_legal_moves();
}

std::vector<Move> SearchPosition::generate_legal_moves() const {
    return m_board.generate_legal_moves();
}

bool SearchPosition::in_check(const PlayerColor side) const {
    return m_board.in_check(side);
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
    int delta = 0;

    // Move piece and handle promo
    delta +=  _pst_value(piece, side, to) - _pst_value(piece, side, from);
    if(promo != PieceType::None) {
        delta += _material_value(promo) - _material_value(piece);
    }

    // Handle capture and en passant
    if(captured != PieceType::None) {
        int capture_square = is_ep ? (from & 0b111000 /* rank of from */) | (to & 0b000111 /* file of to */) : to;
        delta += _material_value(captured);
        delta += _pst_value(captured, opponent(side), capture_square);
    }

    // Handle castling
    if(is_castle){
        int rook_from = to > from ? from + 3 : from - 4;
        int rook_to = (to + from) >> 1;
        delta += _pst_value(PieceType::Rook, side, rook_to) + _pst_value(PieceType::Rook, side, rook_from);
    }

    m_eval_history.push_back(m_eval);
    m_eval += side == PlayerColor::White ? delta : -delta;
    m_board.make_move(move);
}

bool SearchPosition::undo_move() {
    if(m_eval_history.size() == 0) return false;
    m_eval = m_eval_history.back();
    m_eval_history.pop_back();
    m_board.undo_move();
    return true;
}

Move SearchPosition::move_from_uci(const UCI& uci) const {
    return m_board.move_from_uci(uci);
}

int SearchPosition::_material_value(PieceType type) const {
    switch(type) {
        case PieceType::Pawn:   return 100;
        case PieceType::Knight: return 320;
        case PieceType::Bishop: return 330;
        case PieceType::Rook:   return 500;
        case PieceType::Queen:  return 900;
        default: return 0;
    }
}

int SearchPosition::_pst_value(PieceType type, PlayerColor color, int square) const {
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

int SearchPosition::_compute_full_eval() {
    int eval = 0;

    for (int square = 0; square < 64; square++) {
        Piece piece = m_board.get_piece_at(square);
        if (piece.type == PieceType::None) continue;

        int val = _material_value(piece.type) + _pst_value(piece.type, piece.color, square);
        eval += (piece.color == PlayerColor::White ? val : -val);
    }

    return eval;
}
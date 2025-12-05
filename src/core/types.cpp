#include "core/types.hpp"

static std::string square_name(Square sq) {
    char file = 'a' + (+sq % 8);
    char rank = '1' + (+sq / 8);
    return {file, rank};
}

UCI MoveEncoding::to_uci(const Move& move) {
    Square from = MoveEncoding::from_sq(move);
    Square to = MoveEncoding::to_sq(move);
    MoveType move_type = MoveEncoding::move_type(move);
    
    std::string uci = square_name(from) + square_name(to);
    if (move_type != MoveType::Promotion) {
        return uci;
    }

    PieceType promo = MoveEncoding::promo(move);
    switch (promo) {
        case PieceType::Queen:  uci += 'q'; break;
        case PieceType::Rook:   uci += 'r'; break;
        case PieceType::Bishop: uci += 'b'; break;
        case PieceType::Knight: uci += 'n'; break;
        default: assert(false); // should never be hit
    }

    return uci; // GCOVR_EXCL_LINE
}

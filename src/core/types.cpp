#include "core/types.hpp"

Piece::Piece() : type(PieceType::None), color(PlayerColor::White) {}

Piece::Piece(PieceType type_, PlayerColor color_) : type(type_), color(color_) {}

bool Piece::operator==(const Piece& other) const {
    return type == other.type && color == other.color;
}
bool Piece::operator!=(const Piece& other) const {
    return !operator==(other);
}

std::size_t Piece::Hash::operator()(const Piece& p) const {
    return static_cast<std::size_t>(p.type) * 4 + static_cast<std::size_t>(p.color);
}

std::string square_name(int sq) {
    char file = 'a' + (sq % 8);
    char rank = '1' + (sq / 8);
    return {file, rank};
}

UCI MoveEncoding::to_uci(const Move& move) {
    int from = MoveEncoding::from_sq(move);
    int to   = MoveEncoding::to_sq(move);
    PieceType promo = MoveEncoding::promo(move);

    std::string uci = square_name(from) + square_name(to);

    switch (promo) {
        case PieceType::Queen:  uci += 'q'; break;
        case PieceType::Rook:   uci += 'r'; break;
        case PieceType::Bishop: uci += 'b'; break;
        case PieceType::Knight: uci += 'n'; break;
        default: break;
    }

    return uci;
}

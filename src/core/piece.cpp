#include "core/piece.hpp"

bool Piece::operator==(const Piece& other) const {
    return type == other.type && color == other.color;
}
bool Piece::operator!=(const Piece& other) const {
    return !operator==(other);
}

std::size_t Piece::Hash::operator()(const Piece& p) const {
    return static_cast<std::size_t>(p.type) * 16 + static_cast<std::size_t>(p.color);
}

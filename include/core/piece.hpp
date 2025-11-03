#pragma once

#include <cstddef>

enum class PieceType {
    None = 0,
    Pawn = 1, 
    Knight = 2, 
    Bishop = 3, 
    Rook = 4, 
    Queen = 5, 
    King = 6
};

enum class PlayerColor {
    None = 0,
    White = 1,
    Black = 2
};

struct Piece {
    PieceType type = PieceType::None;
    PlayerColor color = PlayerColor::None;
    bool operator==(const Piece& other) const;
    bool operator!=(const Piece& other) const;
    struct Hash {
        std::size_t operator()(const Piece& p) const;
    };
};

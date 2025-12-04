#pragma once

#include <string>
#include <cstdint>
#include <cassert>

#include "standards.hpp"

enum class Square : int8_t {
    A1 = 0, B1 = 1, C1 = 2, D1 = 3, E1 = 4, F1 = 5, G1 = 6, H1 = 7,
    A2 = 8, B2 = 9, C2 = 10, D2 = 11, E2 = 12, F2 = 13, G2 = 14, H2 = 15,
    A3 = 16, B3 = 17, C3 = 18, D3 = 19, E3 = 20, F3 = 21, G3 = 22, H3 = 23,
    A4 = 24, B4 = 25, C4 = 26, D4 = 27, E4 = 28, F4 = 29, G4 = 30, H4 = 31,
    A5 = 32, B5 = 33, C5 = 34, D5 = 35, E5 = 36, F5 = 37, G5 = 38, H5 = 39,
    A6 = 40, B6 = 41, C6 = 42, D6 = 43, E6 = 44, F6 = 45, G6 = 46, H6 = 47,
    A7 = 48, B7 = 49, C7 = 50, D7 = 51, E7 = 52, F7 = 53, G7 = 54, H7 = 55,
    A8 = 56, B8 = 57, C8 = 58, D8 = 59, E8 = 60, F8 = 61, G8 = 62, H8 = 63, None = 64
};

enum class Shift : int8_t {
    Up = 8,
    DoubleUp = 16,
    Down = -8,
    DoubleDown = -16,
    Left = -1,
    Right = 1,
    UpRight = 9,
    UpLeft = 7,
    DownRight = -7,
    DownLeft = -9
};

constexpr int8_t operator+(Square t) noexcept { return static_cast<int8_t>(t); }
constexpr int8_t operator+(Shift t)  noexcept { return static_cast<int8_t>(t); }

constexpr bool is_valid_square(Square sq) {
    return sq >= Square::A1 && sq <= Square::H8;
}
constexpr Square operator+(Square sq, int i) {
    return static_cast<Square>((+sq) + i);
}
constexpr Square operator-(Square sq, int i) {
    return static_cast<Square>((+sq) - i);
}
constexpr Square operator+(Square sq, Shift sh) {
    return static_cast<Square>((+sq) + (+sh));
}
constexpr Square operator-(Square sq, Shift sh) {
    return static_cast<Square>((+sq) - (+sh));
}
constexpr Square& operator++(Square& sq) noexcept {
    sq = static_cast<Square>(+sq + 1);
    return sq;
}

// Piece features
enum class PieceType : int8_t { Knight = 0, Bishop = 1, Rook = 2, Queen = 3, King = 4, Pawn = 5, None = 6};
enum class Color : int8_t { White = 0, Black = 1 };

// Piece = Color + PieceType
enum class Piece : int8_t {
    WKnight = 0, WBishop = 1, WRook = 2, WQueen = 3, WKing = 4, WPawn = 5,
    BKnight = 8, BBishop = 9, BRook = 10, BQueen = 11, BKing = 12, BPawn = 13,
    None = 14
};
constexpr Piece create_piece(Color color, PieceType type) {
    return static_cast<Piece>(static_cast<int8_t>(type) + (static_cast<int8_t>(color) << 3));
}
constexpr PieceType to_type(Piece p) {
    return static_cast<PieceType>(static_cast<int8_t>(p) & 0x7);
}
constexpr Color to_color(Piece p) {
    return (static_cast<int8_t>(p) < 8) ? Color::White : Color::Black;
}
constexpr int8_t operator+(PieceType t) noexcept { return static_cast<int8_t>(t); }
constexpr int8_t operator+(Color c)     noexcept { return static_cast<int8_t>(c); }
constexpr int8_t operator+(Piece p)     noexcept { return static_cast<int8_t>(p); }

// More helpers
constexpr inline Square square_for_side(Square square, Color side) { return (side == Color::White) ? square : (square + -63); };
constexpr Color opponent(Color side)                 { return side == Color::White ? Color::Black : Color::White; };
constexpr Square create_square(int file, int rank)   { return static_cast<Square>(rank * 8 + file); }
constexpr int square_index(int file, int rank)       { return rank * 8 + file;  }
constexpr int rank_of(Square square)                 { return +square / 8; }
constexpr int file_of(Square square)                 { return +square % 8; }
constexpr int rank_of_relative(Square square, Color side) {
    return side == Color::White ? rank_of(square) : 7 - rank_of(square);
}
constexpr int file_of_relative(Square square, Color side) {
    return side == Color::White ? file_of(square) : 7 - file_of(square);
}

// Move encoding to 16 bits
// Bits 0-5:   from square (0-63)
// Bits 6-11:  to square (0-63)
// Bits 12-13: promoted piece type
// Bit  14-15: MoveType
using Move = uint16_t;

enum class MoveType : int8_t {
    Normal = 0,
    Promotion = 1,
    Castle = 2,
    EnPassant = 3
};
constexpr int8_t operator+(MoveType t)  noexcept { return static_cast<int8_t>(t); }

namespace MoveEncoding {
    /**
     * Encode move description into 16-bits.
     * @param mt move type
     * @param from origin square 0-63 (8 * rank + file).
     * @param to target square 0-63 (8 * rank + file).
     * @param promo promotion piece type (only for promotion moves).
     */
    template<MoveType mt>
    constexpr Move encode(Square from, Square to, PieceType promo = PieceType::None) {
        if constexpr (mt == MoveType::Normal) {
            return (+from) | (+to << 6);
        } else if constexpr (mt == MoveType::Promotion) {
            assert(promo == PieceType::Knight || promo == PieceType::Bishop
                   || promo == PieceType::Rook || promo == PieceType::Queen);
            return (+from) | (+to << 6) | (+promo << 12) | (+mt << 14);
        } else {
            return (+from) | (+to << 6) | (+mt << 14);
        }
    }

    /**
     * @return Origin square 0-63 (8 * rank + file).
     */
    constexpr Square from_sq(Move m) { return static_cast<Square>(m & 0x3F); }

    /**
     * @return Target square 0-63 (8 * rank + file).
     */
    constexpr Square to_sq(Move m) { return static_cast<Square>((m >> 6) & 0x3F); }

    /**
     * @return Promotion piece type.
     */
    constexpr PieceType promo(Move m) { return static_cast<PieceType>((m >> 12) & 0x3); }

    /**
     * @return Move type (normal, promotion, castle, en passant).
     */
    constexpr MoveType move_type(Move m) { return static_cast<MoveType>((m >> 14) & 0x3); }

    /**
     * @return UCI representation of the move.
     */
    UCI to_uci(const Move& move);
}

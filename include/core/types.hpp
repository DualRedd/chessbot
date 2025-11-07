#pragma once

#include <string>

// UCI standard
using UCI = std::string;

// FEN standard
using FEN = std::string;

// Move encoding (32 bits)
using Move = uint32_t;

/**
 * White or black side.
 */
enum class PlayerColor : uint8_t { White = 0, Black = 1 };

/**
 * Conversion to integer.
 */
constexpr int operator+(PlayerColor c) noexcept {
    return static_cast<int>(c);
}

/**
 * Chess piece type.
 */
enum class PieceType : uint8_t { Pawn = 0, Knight = 1, Bishop = 2, Rook = 3, Queen = 4, King = 5, None = 6};

/**
 * Conversion to integer.
 */
constexpr int operator+(PieceType t) noexcept {
    return static_cast<int>(t);
}

/**
 * Combines piece type and color.
 */
struct Piece {
    PieceType type;
    PlayerColor color;

    Piece();
    Piece(PieceType type, PlayerColor color);

    bool operator==(const Piece& other) const;
    bool operator!=(const Piece& other) const;

    struct Hash {
        std::size_t operator()(const Piece& p) const;
    };
};

/**
 * Move encoding & decoding helpers
 */
namespace MoveEncoding {
    /**
     * Encode move description into 32-bits.
     */
    constexpr Move encode(int from, int to, PieceType piece, PieceType capture = PieceType::None,
                      PieceType promo = PieceType::None, bool is_castle = false, bool is_ep = false) {
        return (from)
             | (to << 6)
             | (static_cast<int>(piece) << 12)
             | (static_cast<int>(capture) << 15)
             | (static_cast<int>(promo) << 18)
             | (is_castle ? 1 << 21 : 0)
             | (is_ep ? 1 << 22 : 0);
    }

    /**
     * @return Origin square 0-63 (8 * rank + file).
     */
    constexpr int from_sq(Move m) { return m & 0x3F; }

    /**
     * @return Target square 0-63 (8 * rank + file).
     */
    constexpr int to_sq(Move m) { return (m >> 6) & 0x3F; }

    /**
     * @return Moved piece type.
     */
    constexpr PieceType piece(Move m) { return static_cast<PieceType>((m >> 12) & 0x7); }

    /**
     * @return Captured piece type.
     */
    constexpr PieceType capture(Move m) { return static_cast<PieceType>((m >> 15) & 0x7); }

    /**
     * @return Promotion piece type.
     */
    constexpr PieceType promo(Move m) { return static_cast<PieceType>((m >> 18) & 0x7); }

    /**
     * @return Whether this move is a castle.
     */
    constexpr bool is_castle(Move m) { return static_cast<bool>((m >> 21) & 1); }

    /**
     * @return Whether this move is an en passant.
     */
    constexpr bool is_en_passant(Move m) { return static_cast<bool>((m >> 22) & 1); }

    /**
     * @return UCI representation of the move.
     */
    UCI to_uci(const Move& move);
}

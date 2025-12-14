#pragma once

// Export macro for Windows DLLs
#if defined(_WIN32)
  #if defined(CHESS_CORE_EXPORTS)
    #define API __declspec(dllexport)
  #else
    #define API __declspec(dllimport)
  #endif
#else
  #define API
#endif

#include "types.hpp"

#if defined(PEXT_ENABLED) && defined(__BMI2__) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#define USE_PEXT
#endif
#ifdef _MSC_VER
#include <intrin.h>
#endif

// ----------------
// Type definitions
// ----------------

// Bitboard type (64 bits)
using Bitboard = uint64_t;

// Convert bitboard to string representation
std::string to_string(Bitboard bb);

enum class CastlingSide : int8_t { KingSide = 0, QueenSide = 1 };
enum class CastlingFlag : int8_t {
    WhiteKingSide  = 1 << 0,
    WhiteQueenSide = 1 << 1,
    BlackKingSide  = 1 << 2,
    BlackQueenSide = 1 << 3
};
constexpr int8_t operator+(CastlingSide t) noexcept { return static_cast<int8_t>(t); }
constexpr int8_t operator+(CastlingFlag t) noexcept { return static_cast<int8_t>(t); }

// ------------------------------
// Precalculated masks and values
// ------------------------------

// Square and line masks
extern API Bitboard MASK_SQUARE[64];              // [square]
extern API Bitboard MASK_BETWEEN[64][64];         // [from][to] non-inclusive on both ends
extern API Bitboard MASK_LINE[64][64];            // [from][to] edge to edge
extern API Bitboard MASK_FILE[8];                 // [file]
extern API Bitboard MASK_RANK[8];                 // [rank]

// Non-blocking attack masks
extern API Bitboard MASK_PAWN_ATTACKS[2][64];     // [color][square]
extern API Bitboard MASK_KNIGHT_ATTACKS[64];      // [square]
extern API Bitboard MASK_KING_ATTACKS[64];        // [square]
extern API Bitboard MASK_ROOK_ATTACKS[64];        // [square] non-blocking
extern API Bitboard MASK_BISHOP_ATTACKS[64];      // [square] non-blocking

// Pawn structure masks
extern API Bitboard REAR_SPAN[2][64];             // [color][square]
extern API Bitboard FRONT_SPAN[2][64];            // [color][square]
extern API Bitboard LEFT_ATTACK_FILE_FILL[64];    // [square]
extern API Bitboard RIGHT_ATTACK_FILE_FILL[64];   // [square]

// Magic values and bitboards for sliding pieces
extern API uint64_t ROOK_MAGIC[64];               // [square]
extern API uint64_t BISHOP_MAGIC[64];             // [square]
extern API Bitboard MASK_ROOK_MAGIC[64];          // [square]
extern API Bitboard MASK_BISHOP_MAGIC[64];        // [square]
extern API Bitboard ROOK_ATTACK_TABLE[64][4096];  // [square][index]
extern API Bitboard BISHOP_ATTACK_TABLE[64][512]; // [square][index]

// Castling masks and flags
extern API Bitboard MASK_CASTLE_CLEAR[2][2];      // [color][side]
extern API int8_t MASK_CASTLE_FLAG[64];           // [square]

// Zobrist hashing keys
extern API uint64_t ZOBRIST_PIECE[14][64];        // [piece][square]
extern API uint64_t ZOBRIST_CASTLING[16];         // [castling rights bitmask]
extern API uint64_t ZOBRIST_EP[8];                // [file]
extern API uint64_t ZOBRIST_SIDE;                 // black to move

// Precalculation function
void init_bitboards();

// -------------------------
// Bitboard helper functions
// -------------------------

// Helper functions for castling
constexpr int8_t castling_flag(Color color, CastlingSide side) {
    return int8_t{1} << ((static_cast<int8_t>(color) << 1) + static_cast<int8_t>(side));
}
constexpr Square king_start_square(Color color) {
    return color == Color::White ? Square::E1 : Square::E8;
}

// Helper functions for bit operations
constexpr void pop_lsb(Bitboard &b) { b &= b - 1; }
constexpr bool more_than_1bit(Bitboard b) { return (b & (b - 1)) != 0ULL; }
#ifdef _MSC_VER
inline Square lsb(Bitboard b) { unsigned long index; _BitScanForward64(&index, b); return static_cast<Square>(index); }
inline int popcount(Bitboard b) { return static_cast<int>(__popcnt64(b)); }
#else
inline Square lsb(Bitboard b) { return static_cast<Square>(__builtin_ctzll(b)); }
inline int popcount(Bitboard b) { return static_cast<int>(__builtin_popcountll(b)); }
#endif
#ifdef USE_PEXT
inline uint64_t pext(Bitboard val, Bitboard mask) { return _pext_u64(val, mask); }
#endif

template<Shift shift>
constexpr inline Bitboard shift_bb(Bitboard bb) {
    if constexpr (shift == Shift::Up)              return bb << 8;
    else if constexpr (shift == Shift::DoubleUp)   return bb << 16;
    else if constexpr (shift == Shift::Down)       return bb >> 8;
    else if constexpr (shift == Shift::DoubleDown) return bb >> 16;
    else if constexpr (shift == Shift::Left)       return (bb & ~MASK_FILE[0]) >> 1;
    else if constexpr (shift == Shift::Right)      return (bb & ~MASK_FILE[7]) << 1;
    else if constexpr (shift == Shift::UpRight)    return (bb & ~MASK_FILE[7]) << 9;
    else if constexpr (shift == Shift::UpLeft)     return (bb & ~MASK_FILE[0]) << 7;
    else if constexpr (shift == Shift::DownRight)  return (bb & ~MASK_FILE[7]) >> 7;
    else if constexpr (shift == Shift::DownLeft)   return (bb & ~MASK_FILE[0]) >> 9;
}

/**
 * Get attack bitboard for a piece from a given square, considering occupied squares.
 * @tparam type of piece (except Pawn)
 * @param square Square from which to calculate attacks
 * @param occupied Bitboard of occupied squares
 * @return Bitboard of attack squares
 * @note Pawn attacks depend on color and are to be handled separately.
 */
template<PieceType type>
inline Bitboard attacks_from(Square square, Bitboard occupied) {
    static_assert(type != PieceType::Pawn, "Pawn attacks depend on color and are to be handled separately");

    if constexpr (type == PieceType::Knight) {
        return MASK_KNIGHT_ATTACKS[+square];
    }
#ifdef USE_PEXT
    else if constexpr (type == PieceType::Bishop) {
        uint64_t index = pext(occupied, MASK_BISHOP_MAGIC[+square]);
        return BISHOP_ATTACK_TABLE[+square][index];
    }
    else if constexpr (type == PieceType::Rook) {
        uint64_t index = pext(occupied, MASK_ROOK_MAGIC[+square]);
        return ROOK_ATTACK_TABLE[+square][index];
    }
#else
    else if constexpr (type == PieceType::Bishop) {
        constexpr int bits = 9;
        uint64_t index = ((occupied & MASK_BISHOP_MAGIC[+square]) * BISHOP_MAGIC[+square]) >> (64 - bits);
        return BISHOP_ATTACK_TABLE[+square][index];
        
    }
    else if constexpr (type == PieceType::Rook) {
        constexpr int bits = 12;
        uint64_t index = ((occupied & MASK_ROOK_MAGIC[+square]) * ROOK_MAGIC[+square]) >> (64 - bits);
        return ROOK_ATTACK_TABLE[+square][index];
    }
#endif
    else if constexpr (type == PieceType::Queen) {
        return attacks_from<PieceType::Bishop>(square, occupied)
             | attacks_from<PieceType::Rook>(square, occupied);
    }
    else if constexpr (type == PieceType::King) {
        return MASK_KING_ATTACKS[+square];
    }
}


/**
 * All squares in front of each pawn.
 * @tparam side Color of the pawns
 * @param pawns Bitboard of pawns
 * @return Bitboard of front spans.
 */
template<Color side>
inline Bitboard front_spans(Bitboard pawns) {
    Bitboard spans = 0ULL;
    while (pawns) {
        Square sq = lsb(pawns);
        spans |= FRONT_SPAN[+side][+sq];
        pop_lsb(pawns);
    }
    return spans;
}

/**
 * All squares behind each pawn and the pawn squares themselves.
 * @tparam side Color of the pawns
 * @param pawns Bitboard of pawns
 * @return Bitboard of rear spans.
 */
template<Color side>
inline Bitboard rear_spans(Bitboard pawns) {
    Bitboard spans = 0ULL;
    while (pawns) {
        Square sq = lsb(pawns);
        spans |= REAR_SPAN[+side][+sq];
        pop_lsb(pawns);
    }
    return spans;
}

/**
 * Squares under attack by pawns of the given color.
 * @tparam side Color of the pawns
 * @param pawns Bitboard of pawns
 * @return Bitboard of attack squares.
 */
template<Color side>
inline Bitboard pawn_attacks(Bitboard pawns) {
    return shift_bb<pawn_dir(side)+Shift::Left>(pawns)
         | shift_bb<pawn_dir(side)+Shift::Right>(pawns);
}

/**
 * Attack spans in front of pawns of the given color. Front span shifted left and right.
 * @tparam side Color of the pawns
 * @param pawns Bitboard of pawns
 * @return Bitboard of attack front spans.
 */
template<Color side>
inline Bitboard attack_front_spans(Bitboard pawns) {
    const Bitboard front_spans_bb = front_spans<side>(pawns);
    return shift_bb<Shift::Left>(front_spans_bb) | shift_bb<Shift::Right>(front_spans_bb);
}

/**
 * Attack spans behind pawns of the given color. Rear span shifted left and right.
 * @tparam side Color of the pawns
 * @param pawns Bitboard of pawns
 * @return Bitboard of attack rear spans.
 */
template<Color side>
inline Bitboard attack_rear_spans(Bitboard pawns) {
    const Bitboard rear_spans_bb = rear_spans<side>(pawns);
    return shift_bb<Shift::Left>(rear_spans_bb) | shift_bb<Shift::Right>(rear_spans_bb);
}

/**
 * File fills to the left of pawns of the given color.
 * @param pawns Bitboard of pawns
 * @return Bitboard of left attack file fills.
 */
inline Bitboard left_attack_file_fills(Bitboard pawns) {
    Bitboard fill = 0ULL;
    while (pawns) {
        Square sq = lsb(pawns);
        fill |= LEFT_ATTACK_FILE_FILL[+sq];
        pop_lsb(pawns);
    }
    return fill;
}

/**
 * File fills to the right of pawns of the given color.
 * @param pawns Bitboard of pawns
 * @return Bitboard of right attack file fills.
 */
inline Bitboard right_attack_file_fills(Bitboard pawns) {
    Bitboard fill = 0ULL;
    while (pawns) {
        Square sq = lsb(pawns);
        fill |= RIGHT_ATTACK_FILE_FILL[+sq];
        pop_lsb(pawns);
    }
    return fill;
}

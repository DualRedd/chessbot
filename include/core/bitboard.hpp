#pragma once

#include "types.hpp"

#if defined(PEXT_ENABLED) && defined(__BMI2__) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#define USE_PEXT
#endif

#ifdef _MSC_VER
#include <intrin.h>
#endif

// Bitboard type (64 bits)
using Bitboard = uint64_t;

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

// Precalculated masks and values
extern Bitboard MASK_SQUARE[64];              // [square]
extern Bitboard MASK_BETWEEN[64][64];         // [from square][to square] non-inclusive on both ends
extern Bitboard MASK_LINE[64][64];            // [from square][to square] edge to edge

extern Bitboard MASK_PAWN_ATTACKS[2][64];     // [color][square]
extern Bitboard MASK_KNIGHT_ATTACKS[64];      // [square]
extern Bitboard MASK_KING_ATTACKS[64];        // [square]
extern Bitboard MASK_ROOK_ATTACKS[64];        // [square] non-blocking
extern Bitboard MASK_BISHOP_ATTACKS[64];      // [square] non-blocking

extern uint64_t ROOK_MAGIC[64];               // [square]
extern uint64_t BISHOP_MAGIC[64];             // [square]
extern Bitboard MASK_ROOK_MAGIC[64];          // [square]
extern Bitboard MASK_BISHOP_MAGIC[64];        // [square]
extern Bitboard ROOK_ATTACK_TABLE[64][4096];  // [square][index]
extern Bitboard BISHOP_ATTACK_TABLE[64][512]; // [square][index]

extern Bitboard MASK_CASTLE_CLEAR[2][2];      // [color][0=queenside,1=kingside]
extern int8_t MASK_CASTLE_FLAG[64];           // [square] combination of CastlingFlag to remove when moving from/to this square

extern uint64_t ZOBRIST_PIECE[2][6][64];      // [color][piece][square]
extern uint64_t ZOBRIST_CASTLING[16];         // [castling rights bitmask]
extern uint64_t ZOBRIST_EP[8];                // [file of en passant square]
extern uint64_t ZOBRIST_SIDE;                 // side to move

void init_bitboards();

constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
constexpr Bitboard RANK_2 = 0x000000000000FF00ULL;
constexpr Bitboard RANK_3 = 0x0000000000FF0000ULL;
constexpr Bitboard RANK_4 = 0x00000000FF000000ULL;
constexpr Bitboard RANK_5 = 0x000000FF00000000ULL;
constexpr Bitboard RANK_6 = 0x0000FF0000000000ULL;
constexpr Bitboard RANK_7 = 0x00FF000000000000ULL;
constexpr Bitboard RANK_8 = 0xFF00000000000000ULL;
constexpr Bitboard FILE_A = 0x0101010101010101ULL;
constexpr Bitboard FILE_B = 0x0202020202020202ULL;
constexpr Bitboard FILE_C = 0x0404040404040404ULL;
constexpr Bitboard FILE_D = 0x0808080808080808ULL;
constexpr Bitboard FILE_E = 0x1010101010101010ULL;
constexpr Bitboard FILE_F = 0x2020202020202020ULL;
constexpr Bitboard FILE_G = 0x4040404040404040ULL;
constexpr Bitboard FILE_H = 0x8080808080808080ULL;
constexpr Bitboard PROMOTION_RANKS = RANK_1 | RANK_8;
constexpr Bitboard FULL_BOARD = 0xFFFFFFFFFFFFFFFFULL;

// Helper functions
constexpr int8_t castling_flag(Color color, CastlingSide side) {
    return int8_t{1} << ((static_cast<int8_t>(color) << 1) + static_cast<int8_t>(side));
}
constexpr Square king_start_square(Color color) {
    return color == Color::White ? Square::E1 : Square::E8;
}

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
    else if constexpr (shift == Shift::Left)       return (bb & ~FILE_A) >> 1;
    else if constexpr (shift == Shift::Right)      return (bb & ~FILE_H) << 1;
    else if constexpr (shift == Shift::UpRight)    return (bb & ~FILE_H) << 9;
    else if constexpr (shift == Shift::UpLeft)     return (bb & ~FILE_A) << 7;
    else if constexpr (shift == Shift::DownRight)  return (bb & ~FILE_H) >> 7;
    else if constexpr (shift == Shift::DownLeft)   return (bb & ~FILE_A) >> 9;
    else static_assert(false, "Unknown shift type");
}

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
    else {
        static_assert(false, "Unknown piece type");
    }
}

        
        
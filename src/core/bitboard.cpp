#include "core/bitboard.hpp"

#include <random>

std::string to_string(Bitboard bb) {
    std::string result;
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            Square sq = create_square(file, rank);
            result += (bb & MASK_SQUARE[+sq]) ? '1' : '0';
        }
        if (rank > 0) result += '\n';
    }
    return result;
}

struct Initializer {
    Initializer() { init_bitboards(); }
};
static Initializer _init_once;

Bitboard MASK_SQUARE[64];
Bitboard MASK_PAWN_ATTACKS[2][64];
Bitboard MASK_KNIGHT_ATTACKS[64];
Bitboard MASK_KING_ATTACKS[64];

Bitboard MASK_CASTLE_CLEAR[2][2];
int8_t  MASK_CASTLE_FLAG[64];

uint64_t ZOBRIST_PIECE[2][6][64];
uint64_t ZOBRIST_CASTLING[16];
uint64_t ZOBRIST_EP[8];
uint64_t ZOBRIST_SIDE;

void init_bitboards() {
    // Single square masks
    for (int square = 0; square < 64; square++) {
        MASK_SQUARE[square] = 1ULL << square;
    }

    // Castling masks and flags
    MASK_CASTLE_CLEAR[+Color::White][+CastlingSide::QueenSide] = 0ULL;
    MASK_CASTLE_CLEAR[+Color::Black][+CastlingSide::QueenSide] = 0ULL;
    for (int i = 1; i <= 3; i++) {
        MASK_CASTLE_CLEAR[+Color::White][+CastlingSide::QueenSide] |= MASK_SQUARE[+king_start_square(Color::White) - i];
        MASK_CASTLE_CLEAR[+Color::Black][+CastlingSide::QueenSide] |= MASK_SQUARE[+king_start_square(Color::Black) - i];
    }

    MASK_CASTLE_CLEAR[+Color::White][+CastlingSide::KingSide] = 0ULL;
    MASK_CASTLE_CLEAR[+Color::Black][+CastlingSide::KingSide] = 0ULL;
    for (int i = 1; i <= 2; i++) {
        MASK_CASTLE_CLEAR[+Color::White][+CastlingSide::KingSide] |= MASK_SQUARE[+king_start_square(Color::White) + i];
        MASK_CASTLE_CLEAR[+Color::Black][+CastlingSide::KingSide] |= MASK_SQUARE[+king_start_square(Color::Black) + i];
    }

    MASK_CASTLE_FLAG[+Square::A1] = +CastlingFlag::WhiteQueenSide;
    MASK_CASTLE_FLAG[+Square::H1] = +CastlingFlag::WhiteKingSide;
    MASK_CASTLE_FLAG[+Square::A8] = +CastlingFlag::BlackQueenSide;
    MASK_CASTLE_FLAG[+Square::H8] = +CastlingFlag::BlackKingSide;
    MASK_CASTLE_FLAG[+king_start_square(Color::White)] = +CastlingFlag::WhiteKingSide | +CastlingFlag::WhiteQueenSide;
    MASK_CASTLE_FLAG[+king_start_square(Color::Black)] = +CastlingFlag::BlackKingSide | +CastlingFlag::BlackQueenSide;
    
    // Piece masks
    for (Square square = Square::A1; square <= Square::H8; ++square) {
        File file = file_of(square);
        Rank rank = rank_of(square);

        // Pawn attacks
        MASK_PAWN_ATTACKS[+Color::White][+square] = 0ULL;
        MASK_PAWN_ATTACKS[+Color::Black][+square] = 0ULL;
        if (+rank < 7) {
            if (+file > 0) MASK_PAWN_ATTACKS[+Color::White][+square] |= MASK_SQUARE[+create_square(+file - 1, +rank + 1)];
            if (+file < 7) MASK_PAWN_ATTACKS[+Color::White][+square] |= MASK_SQUARE[+create_square(+file + 1, +rank + 1)];
        }
        if (+rank > 0) {
            if (+file > 0) MASK_PAWN_ATTACKS[+Color::Black][+square] |= MASK_SQUARE[+create_square(+file - 1, +rank - 1)];
            if (+file < 7) MASK_PAWN_ATTACKS[+Color::Black][+square] |= MASK_SQUARE[+create_square(+file + 1, +rank - 1)];
        }

        // Knights
        MASK_KNIGHT_ATTACKS[+square] = 0ULL;
        const int knight_offsets[8][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
        for (auto [df, dr] : knight_offsets) {
            if (+file + df >= 0 && +file + df < 8 && +rank + dr >= 0 && +rank + dr < 8) {
                MASK_KNIGHT_ATTACKS[+square] |= MASK_SQUARE[+create_square(+file + df, +rank + dr)];
            }
        }

        // King
        MASK_KING_ATTACKS[+square] = 0ULL;
        for (int dr = -1; dr <= 1; dr++) {
            for (int df = -1; df <= 1; df++) {
                if (dr == 0 && df == 0) continue;
                if (+file + df >= 0 && +file + df < 8 && +rank + dr >= 0 && +rank + dr < 8) {
                    MASK_KING_ATTACKS[+square] |= MASK_SQUARE[+create_square(+file + df, +rank + dr)];
                }
            }
        }
    }

    // Zobrist hashing keys
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::uniform_int_distribution<uint64_t> distr;

    for (int color = 0; color < 2; ++color) {
        for (int piece = 0; piece < 6; ++piece) {
            for (int square = 0; square < 64; ++square) {
                ZOBRIST_PIECE[color][piece][square] = distr(rng);
            }
        }
    }
    for (int i = 0; i < 16; ++i) {
        ZOBRIST_CASTLING[i] = distr(rng);
    }
    for (int f = 0; f < 8; ++f) {
        ZOBRIST_EP[f] = distr(rng);
    }
    ZOBRIST_SIDE = distr(rng);
}

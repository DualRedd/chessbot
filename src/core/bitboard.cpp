#include "core/bitboard.hpp"

#include <random>
#include <cstring>

Bitboard MASK_SQUARE[64];
Bitboard MASK_BETWEEN[64][64];
Bitboard MASK_LINE[64][64];

Bitboard MASK_PAWN_ATTACKS[2][64];
Bitboard MASK_KNIGHT_ATTACKS[64];
Bitboard MASK_KING_ATTACKS[64];
Bitboard MASK_ROOK_ATTACKS[64];
Bitboard MASK_BISHOP_ATTACKS[64];

Bitboard MASK_CASTLE_CLEAR[2][2];
int8_t  MASK_CASTLE_FLAG[64];

uint64_t ZOBRIST_PIECE[2][6][64];
uint64_t ZOBRIST_CASTLING[16];
uint64_t ZOBRIST_EP[8];
uint64_t ZOBRIST_SIDE;

struct Initializer {
    Initializer() { init_bitboards(); }
};
static Initializer _init_once;

static void zero_tables() {
    std::memset(MASK_SQUARE, 0, sizeof(MASK_SQUARE));
    std::memset(MASK_BETWEEN, 0, sizeof(MASK_BETWEEN));
    std::memset(MASK_LINE, 0, sizeof(MASK_LINE));

    std::memset(MASK_PAWN_ATTACKS, 0, sizeof(MASK_PAWN_ATTACKS));
    std::memset(MASK_KNIGHT_ATTACKS, 0, sizeof(MASK_KNIGHT_ATTACKS));
    std::memset(MASK_KING_ATTACKS, 0, sizeof(MASK_KING_ATTACKS));
    std::memset(MASK_ROOK_ATTACKS, 0, sizeof(MASK_ROOK_ATTACKS));
    std::memset(MASK_BISHOP_ATTACKS, 0, sizeof(MASK_BISHOP_ATTACKS));

    std::memset(MASK_CASTLE_CLEAR, 0, sizeof(MASK_CASTLE_CLEAR));
    std::memset(MASK_CASTLE_FLAG, 0, sizeof(MASK_CASTLE_FLAG));

    std::memset(ZOBRIST_PIECE, 0, sizeof(ZOBRIST_PIECE));
    std::memset(ZOBRIST_CASTLING, 0, sizeof(ZOBRIST_CASTLING));
    std::memset(ZOBRIST_EP, 0, sizeof(ZOBRIST_EP));
    ZOBRIST_SIDE = 0;
}

void init_bitboards() {
    zero_tables();

    // Single square masks
    for (int square = 0; square < 64; square++) {
        MASK_SQUARE[square] = 1ULL << square;
    }

    // Castling masks and flags
    for (int i = 1; i <= 3; i++) {
        MASK_CASTLE_CLEAR[+Color::White][+CastlingSide::QueenSide] |= MASK_SQUARE[+king_start_square(Color::White) - i];
        MASK_CASTLE_CLEAR[+Color::Black][+CastlingSide::QueenSide] |= MASK_SQUARE[+king_start_square(Color::Black) - i];
    }

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
    auto ok = [](int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; };
    for (int from = 0; from < 64; from++) {
        int from_file = from % 8;
        int from_rank = from / 8;

        // Pawn attacks
        if (from_rank < 7) {
            if (from_file > 0) MASK_PAWN_ATTACKS[+Color::White][from] |= MASK_SQUARE[+create_square(from_file - 1, from_rank + 1)];
            if (from_file < 7) MASK_PAWN_ATTACKS[+Color::White][from] |= MASK_SQUARE[+create_square(from_file + 1, from_rank + 1)];
        }
        if (from_rank > 0) {
            if (from_file > 0) MASK_PAWN_ATTACKS[+Color::Black][from] |= MASK_SQUARE[+create_square(from_file - 1, from_rank - 1)];
            if (from_file < 7) MASK_PAWN_ATTACKS[+Color::Black][from] |= MASK_SQUARE[+create_square(from_file + 1, from_rank - 1)];
        }

        // Knights
        const int knight_offsets[8][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
        for (auto [df, dr] : knight_offsets) {
            if (ok(from_file + df, from_rank + dr)) {
                MASK_KNIGHT_ATTACKS[from] |= MASK_SQUARE[+create_square(from_file + df, from_rank + dr)];
            }
        }

        // King
        for (int dr = -1; dr <= 1; dr++) {
            for (int df = -1; df <= 1; df++) {
                if (dr == 0 && df == 0) continue;
                if (ok(from_file + df, from_rank + dr)) {
                    MASK_KING_ATTACKS[from] |= MASK_SQUARE[+create_square(from_file + df, from_rank + dr)];
                }
            }
        }

        // Rook
        const int rook_directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (auto [df, dr] : rook_directions) {
            int cur_file = from_file + df;
            int cur_rank = from_rank + dr;
            while (ok(cur_file, cur_rank)) {
                MASK_ROOK_ATTACKS[from] |= MASK_SQUARE[+create_square(cur_file, cur_rank)];
                cur_file += df;
                cur_rank += dr;
            }
        }

        // Bishop
        const int bishop_directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        for (auto [df, dr] : bishop_directions) {
            int cur_file = from_file + df;
            int cur_rank = from_rank + dr;
            while (ok(cur_file, cur_rank)) {
                MASK_BISHOP_ATTACKS[from] |= MASK_SQUARE[+create_square(cur_file, cur_rank)];
                cur_file += df;
                cur_rank += dr;
            }
        }
    }

    const int directions[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1}};

    // Line and between masks
    for (int from = 0; from < 64; from++) {
        int from_file = from % 8;
        int from_rank = from / 8;
        for (int to = 0; to < 64; ++to) {
            if (to == from) continue;

            int to_file = to % 8;
            int to_rank = to / 8;
            int df = to_file - from_file;
            int dr = to_rank - from_rank;

            int x = (df > 0) - (df < 0);
            int y = (dr > 0) - (dr < 0);

            // aligned if same file, same rank or same diagonal
            if (df == 0 || dr == 0 || (abs(df) == abs(dr))) {
                // build BETWEEN (exclusive)
                Bitboard between = 0ULL;
                int cx = from_file + x;
                int cy = from_rank + y;
                while (cx != to_file || cy != to_rank) {
                    between |= MASK_SQUARE[+create_square(cx, cy)];
                    cx += x, cy += y;
                }
                
                // build BETWEEN (edge to edge)
                Bitboard line = 0ULL;
                cx = from_file;
                cy = from_rank;
                while (ok(cx, cy)) {
                    line |= MASK_SQUARE[+create_square(cx, cy)];
                    cx += x; cy += y;
                }
                cx = from_file - x;
                cy = from_rank - y;
                while (ok(cx, cy)) {
                    line |= MASK_SQUARE[+create_square(cx, cy)];
                    cx -= x; cy -= y;
                }

                MASK_BETWEEN[from][to] = between;
                MASK_LINE[from][to] = line;
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

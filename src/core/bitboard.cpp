#include "core/bitboard.hpp"

#include <random>
#include <cstring>
#include <stdexcept>
#include <iostream>

static std::mt19937_64 rng(42);

Bitboard MASK_SQUARE[64];
Bitboard MASK_BETWEEN[64][64];
Bitboard MASK_LINE[64][64];

Bitboard MASK_PAWN_ATTACKS[2][64];
Bitboard MASK_KNIGHT_ATTACKS[64];
Bitboard MASK_KING_ATTACKS[64];
Bitboard MASK_ROOK_ATTACKS[64];
Bitboard MASK_BISHOP_ATTACKS[64];

uint64_t ROOK_MAGIC[64];
uint64_t BISHOP_MAGIC[64];
Bitboard MASK_ROOK_MAGIC[64];
Bitboard MASK_BISHOP_MAGIC[64];
Bitboard ROOK_ATTACK_TABLE[64][4096];
Bitboard BISHOP_ATTACK_TABLE[64][512];

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

// Precalculation function before magics are initialized
template<Shift shift>
static Bitboard sliding_attacks(Square square, Bitboard occupied) {
    Bitboard attacks = 0ULL;
    Bitboard ray = shift_bb<shift>(MASK_SQUARE[+square]);
    while (ray) {
        attacks |= ray;
        if (ray & occupied) break;
        ray = shift_bb<shift>(ray);
    }
    return attacks;
}

/**
 * Create attack table for sliding pieces (rook or bishop) from a given square.
 * @tparam type of piece (Rook or Bishop)
 * @param sq Square from which to calculate attacks
 */
template<PieceType type>
static void create_attack_table(Square sq) {
    static_assert(type == PieceType::Rook || type == PieceType::Bishop, "create_attack_table is only for rook or bishop");
    assert(MASK_SQUARE[0] == 1); // initialization required for sliding_attacks()

    constexpr int bits = type == PieceType::Rook ? 12 : 9;
    
    // Build relevant occupancy mask
    Bitboard relevant_mask = 0ULL;
    int file = file_of(sq), rank = rank_of(sq);
    if constexpr (type == PieceType::Bishop) {
        for (int f = file + 1, r = rank + 1; f <= 6 && r <= 6; f++, r++) relevant_mask |= 1ULL << (f + r * 8);
        for (int f = file - 1, r = rank + 1; f >= 1 && r <= 6; f--, r++) relevant_mask |= 1ULL << (f + r * 8);
        for (int f = file + 1, r = rank - 1; f <= 6 && r >= 1; f++, r--) relevant_mask |= 1ULL << (f + r * 8);
        for (int f = file - 1, r = rank - 1; f >= 1 && r >= 1; f--, r--) relevant_mask |= 1ULL << (f + r * 8);
    } else if constexpr (type == PieceType::Rook) {
        for (int f = file + 1; f <= 6; f++) relevant_mask |= 1ULL << (f + rank * 8);
        for (int f = file - 1; f >= 1; f--) relevant_mask |= 1ULL << (f + rank * 8);
        for (int r = rank + 1; r <= 6; r++) relevant_mask |= 1ULL << (file + r * 8);
        for (int r = rank - 1; r >= 1; r--) relevant_mask |= 1ULL << (file + r * 8);
    }
    
    // Generate all subsets of the relevant occupancy mask
    std::vector<Bitboard> subsets(1 << popcount(relevant_mask));
    subsets[0] = relevant_mask;
    for (size_t i = 1; i < subsets.size(); i++) {
        subsets[i] = (subsets[i-1] - 1) & relevant_mask;
    }

#ifndef USE_PEXT
    // Find magic number
    uint64_t magic;
    bool collision;
    std::vector<uint32_t> used(1 << bits, 0);

    for(uint32_t i = 1; i < 10'000'000; i++) {
        magic = rng() & rng() & rng(); // low 1 bit count
        if(popcount((relevant_mask * magic) & 0xFF00000000000000ULL) < 6) continue;

        collision = false;
        used.assign(used.size(), 0);

        for (Bitboard occ : subsets) {
            uint64_t index = (occ * magic) >> (64 - bits);
            if (used[index] == i) {
                collision = true;
                break;
            }
            used[index] = i;
        }

        if (!collision) break;
    }
    
    if (collision) {
        throw std::runtime_error("Failed to find magic number in 10 million attempts.");
    }
    
    if constexpr (type == PieceType::Rook) {
        ROOK_MAGIC[+sq] = magic;
    } else {
        BISHOP_MAGIC[+sq] = magic;
    }

#endif
    
    if constexpr (type == PieceType::Rook) {
        MASK_ROOK_MAGIC[+sq] = relevant_mask;
        for (Bitboard occ : subsets) {
#ifdef USE_PEXT
            uint64_t index = pext(occ, relevant_mask);
#else
            uint64_t index = (occ * magic) >> (64 - bits);
#endif
            ROOK_ATTACK_TABLE[+sq][index] = sliding_attacks<Shift::Up>(sq, occ)
                                            | sliding_attacks<Shift::Down>(sq, occ)
                                            | sliding_attacks<Shift::Left>(sq, occ)
                                            | sliding_attacks<Shift::Right>(sq, occ);
        }
    } else {
        MASK_BISHOP_MAGIC[+sq] = relevant_mask;
        for (Bitboard occ : subsets) {
#ifdef USE_PEXT
            uint64_t index = pext(occ, relevant_mask);
#else
            uint64_t index = (occ * magic) >> (64 - bits);
#endif
            BISHOP_ATTACK_TABLE[+sq][index] = sliding_attacks<Shift::UpRight>(sq, occ)
                                            | sliding_attacks<Shift::UpLeft>(sq, occ)
                                            | sliding_attacks<Shift::DownRight>(sq, occ)
                                            | sliding_attacks<Shift::DownLeft>(sq, occ);
        }
    }
}

static void zero_tables() {
    std::memset(MASK_SQUARE, 0, sizeof(MASK_SQUARE));
    std::memset(MASK_BETWEEN, 0, sizeof(MASK_BETWEEN));
    std::memset(MASK_LINE, 0, sizeof(MASK_LINE));

    std::memset(MASK_PAWN_ATTACKS, 0, sizeof(MASK_PAWN_ATTACKS));
    std::memset(MASK_KNIGHT_ATTACKS, 0, sizeof(MASK_KNIGHT_ATTACKS));
    std::memset(MASK_KING_ATTACKS, 0, sizeof(MASK_KING_ATTACKS));
    std::memset(MASK_ROOK_ATTACKS, 0, sizeof(MASK_ROOK_ATTACKS));
    std::memset(MASK_BISHOP_ATTACKS, 0, sizeof(MASK_BISHOP_ATTACKS));

    std::memset(ROOK_MAGIC, 0, sizeof(ROOK_MAGIC));
    std::memset(BISHOP_MAGIC, 0, sizeof(BISHOP_MAGIC));
    std::memset(MASK_ROOK_MAGIC, 0, sizeof(MASK_ROOK_MAGIC));
    std::memset(MASK_BISHOP_MAGIC, 0, sizeof(MASK_BISHOP_MAGIC));
    std::memset(ROOK_ATTACK_TABLE, 0, sizeof(ROOK_ATTACK_TABLE));
    std::memset(BISHOP_ATTACK_TABLE, 0, sizeof(BISHOP_ATTACK_TABLE));

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
    for (int color = 0; color < 2; ++color) {
        for (int piece = 0; piece < 6; ++piece) {
            for (int square = 0; square < 64; ++square) {
                ZOBRIST_PIECE[color][piece][square] = rng();
            }
        }
    }
    for (int i = 0; i < 16; ++i) {
        ZOBRIST_CASTLING[i] = rng();
    }
    for (int f = 0; f < 8; ++f) {
        ZOBRIST_EP[f] = rng();
    }
    ZOBRIST_SIDE = rng();

    // Magics
    rng.seed(3044); // 23922 collision tests required using this seed
    for (int from = 0; from < 64; from++) {
        create_attack_table<PieceType::Rook>(Square(from));
    }
    rng.seed(39024); // 935 collision tests required using this seed
    for (int from = 0; from < 64; from++) {
        create_attack_table<PieceType::Bishop>(Square(from));
    }
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

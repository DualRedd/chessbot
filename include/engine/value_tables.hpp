#pragma once

#include <cstdint>
#include <array>

// ----------
// Game Phase
// ----------
enum class GamePhase { Middlegame = 0, Endgame = 1 };
constexpr int operator+(GamePhase stage) noexcept { return static_cast<int>(stage); }

// Material weights for game phase calculation
constexpr int32_t MATERIAL_WEIGHTS[6] = {
    2,  // Knight
    2,  // Bishop
    3,  // Rook
    5,  // Queen
    0,  // King
    1   // Pawn
};

// Maximum phase value (starting material) = 54
constexpr int32_t PHASE_MAX = MATERIAL_WEIGHTS[+PieceType::Queen]  * 2
                            + MATERIAL_WEIGHTS[+PieceType::Rook]   * 4
                            + MATERIAL_WEIGHTS[+PieceType::Bishop] * 4
                            + MATERIAL_WEIGHTS[+PieceType::Knight] * 4
                            + MATERIAL_WEIGHTS[+PieceType::Pawn]   * 16;

// Minimum phase value (where it is considered completely an endgame for evaluation purposes)
constexpr int32_t PHASE_MIN = 8;

// Phase upper bounds for specific evaluation adjustments
constexpr int32_t PHASE_OPENING = PHASE_MAX;
constexpr int32_t PHASE_EARLY_MIDGAME = 46;
constexpr int32_t PHASE_LATE_MIDGAME = 26;
constexpr int32_t PHASE_EARLY_ENDGAME = 15;
constexpr int32_t PHASE_LATE_ENDGAME = 7;

// Phase width
constexpr int32_t PHASE_WIDTH = PHASE_MAX - PHASE_MIN;

// ------------
// Piece Values
// ------------

constexpr int32_t PIECE_VALUES[8] = {
    320, // Knight
    330, // Bishop
    500, // Rook
    900, // Queen
    0,   // King (handled separately in eval)
    100, // Pawn
    0,   // All
    0    // None
};

constexpr int32_t BISHOP_PAIR_VALUE[2] = {30, 60}; // [gamephase]
constexpr int32_t KNIGHT_PAIR_VALUE[2] = {40, 10}; // [gamephase]
constexpr int32_t KNIGHT_OUTPOST_VALUE[2] = {30, 20}; // [gamephase]


// --------
// Mobility
// --------

constexpr int32_t MOBILITY_VALUES[4][2] = { // [piece][gamephase]
    {4, 5}, // Knight
    {4, 4}, // Bishop
    {0, 4}, // Rook
    {0, 2}, // Queen
};

// -----------
// King Safety
// -----------

constexpr int32_t KING_PAWN_SHIELD_VALUES[2] = {10, 0}; // [gamephase]

// Value per attack type
constexpr int32_t ATTACK_VALUES[4][2] = { // [piece][gamephase]
    {20, 10}, // Knight
    {20, 10}, // Bishop
    {40, 20}, // Rook
    {80, 40}, // Queen
};

// Multiplier based on number of attacks
constexpr int32_t ATTACK_COUNT_MULTIPLIER[7] = {
    0, 50, 75, 88, 94, 97, 100
};

// --------------
// Pawn Structure
// --------------

constexpr int32_t DEFENDED_PAWN_VALUE = 4;
constexpr int32_t DOUBLED_PAWN_VALUE = -20;
constexpr int32_t TRIPLED_PAWN_VALUE = -50;
constexpr int32_t BACKWARD_PAWN_VALUE = -12;
constexpr int32_t ISOLATED_PAWN_VALUES[] = {-14, -14, -16, -20, -20, -16, -14, -14}; // by file
constexpr int32_t PASSED_PAWN_VALUES[] = {0, 0, 14, 24, 40, 60, 80, 0}; // by rank

// -------------------
// Piece-Square Tables
// -------------------

constexpr int32_t PST_PAWN[2][64] = {{ // [gamephase][square]
      0,   0,   0,   0,   0,   0,   0,   0,
    -15, -15, -15, -15, -15, -15, -15, -15,
    -15, -15, -15,   2,   2, -15, -10, -15,
    -15, -10,  -5,  20,  20,  -5, -10, -15,
    -20, -20,   5,  35,  35,   5, -20, -20,
    -20, -20,  15,  30,  30,  15, -20, -20,
    -18, -15,  10,  10,  10,  10, -15, -18,
      0,   0,   0,   0,   0,   0,   0,   0
    },{
      0,   0,   0,   0,   0,   0,   0,   0,
      5,  -5,  -5, -15, -15,  -5,  -5,   5,
      5,  -5,  -5,   4,   4,  -5,  -5,   5,
     10,   6,   6,   8,   8,   6,   6,  10,
     10,  14,  10,  12,  12,  10,  14,  10,
     15,  20,  12,  14,  14,  12,  20,  15,
     20,  25,  23,  18,  18,  23,  25,  20,
      0,   0,   0,   0,   0,   0,   0,   0
}};

constexpr int32_t PST_KNIGHT[2][64] = {{ // [gamephase][square]
    -75, -27, -30, -25, -25, -30, -27, -75,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   5,  10,  20,  20,  10,   5, -30,
    -26,   2,  15,  16,  16,  15,   2, -26,
    -26,   5,  15,  21,  21,  15,   5, -26,
    -30,   2,  10,  16,  16,  10,   2, -30,
    -40, -20, -12, -10, -10, -12, -20, -40,
    -60, -40, -35, -30, -30, -35, -40, -60
    },{
    -50, -30, -20, -16, -16, -20, -30, -50,
    -30, -20,   1,   5,   5,   1, -20, -30,
    -20,   1,   9,  15,  15,   9,   1, -20,
    -16,   5,  15,  21,  21,  15,   5, -16,
    -16,   5,  15,  21,  21,  15,   5, -16,
    -20,   1,   9,  15,  15,   9,   1, -20,
    -30, -20,   1,   5,   5,   1, -20, -30,
    -50, -30, -20, -16, -16, -20, -30, -50
}};


constexpr int32_t PST_BISHOP[2][64] = {{ // [gamephase][square]
    -20, -10, -40, -10, -10,- 40, -10, -20,
    -10,  15,   0,   0,   0,   0,  15, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10,- 10, -10, -20
    },{
    -21, -10, -10, -12, -12, -10, -10, -21,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   4,   5,   5,   4,   0, -10,
    -12,   0,   5,  10,  10,   5,   0, -12,
    -12,   0,   5,  10,  10,   5,   0, -12,
    -10,   0,   4,   5,   5,   4,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -21, -10, -10, -12, -12, -10, -10, -21
}};


constexpr int32_t PST_ROOK[2][64] = {{ // [gamephase][square]
    -15, -10,  -3,   8,   8,  -3, -10, -15,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10, -10,   0,   0,   0,   0, -10, -10,
    -10, -10,   0,   0,   0,   0, -10, -10,
    -10, -10,   0,   0,   0,   0, -10, -10,
    -10, -10,   0,   0,   0,   0, -10, -10,
     10,  10,  10,  10,  10,  10,  10,  10,
      0,   0,   0,   0,   0,   0,   0,   0
    },{
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
}};

constexpr int32_t PST_QUEEN[2][64] = {{ // [gamephase][square]
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
    },{
    -55, -30, -30, -25, -25, -30, -30, -55,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -50, -40, -30, -22, -22, -30, -40, -50
}};

constexpr int32_t PST_KING[2][64] = {{ // [gamephase][square]
    -35, -38, -40, -55, -55, -40, -38, -35,
    -35, -40, -40, -50, -50, -40, -40, -35,
    -35, -40, -40, -50, -50, -40, -40, -35,
    -30, -40, -50, -60, -60, -50, -40, -30,
    -20, -30, -35, -40, -40, -35, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,  -5,  -5,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20
    },{
    -50, -30, -30, -30, -30, -30, -30, -50,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -50, -40, -30, -20, -20, -30, -40, -50
}};

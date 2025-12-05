#include "gtest/gtest.h"
#include "core/bitboard.hpp"

// Minimal tests to verify basic initialization correctness

TEST(BitboardTests, ShiftNoWrap) {
    EXPECT_EQ(shift_bb<Shift::Up>(MASK_SQUARE[+Square::A1]), MASK_SQUARE[+Square::A2]);
    EXPECT_EQ(shift_bb<Shift::UpRight>(MASK_SQUARE[+Square::A1]), MASK_SQUARE[+Square::B2]);
    EXPECT_EQ(shift_bb<Shift::UpLeft>(MASK_SQUARE[+Square::H1]), MASK_SQUARE[+Square::G2]);
    EXPECT_EQ(shift_bb<Shift::DoubleUp>(MASK_SQUARE[+Square::A1]), MASK_SQUARE[+Square::A3]);
    EXPECT_EQ(shift_bb<Shift::DoubleDown>(MASK_SQUARE[+Square::A3]), MASK_SQUARE[+Square::A1]);

    // Literal edge cases
    EXPECT_EQ(shift_bb<Shift::UpLeft>(MASK_SQUARE[+Square::A1]), 0ULL);
    EXPECT_EQ(shift_bb<Shift::Left>(MASK_SQUARE[+Square::A1]), 0ULL);
    EXPECT_EQ(shift_bb<Shift::UpRight>(MASK_SQUARE[+Square::H3]), 0ULL);
    EXPECT_EQ(shift_bb<Shift::Right>(MASK_SQUARE[+Square::H3]), 0ULL);
}

TEST(BitboardTests, KingAttacks) {
    // From E1 king: D1 F1 D2 E2 F2
    Bitboard expected = MASK_SQUARE[+Square::D1] | MASK_SQUARE[+Square::F1] |
                        MASK_SQUARE[+Square::D2] | MASK_SQUARE[+Square::E2] |
                        MASK_SQUARE[+Square::F2];
    EXPECT_EQ(MASK_KING_ATTACKS[+Square::E1], expected);

    // From A1 king: A2 B2 B1
    expected =  MASK_SQUARE[+Square::A2] | MASK_SQUARE[+Square::B2] |
                 MASK_SQUARE[+Square::B1];
    EXPECT_EQ(MASK_KING_ATTACKS[+Square::A1], expected);
}

TEST(BitboardTests, KnightAttacks) {
    // From A1 knight: B3 C2
    Bitboard expected = MASK_SQUARE[+Square::B3] | MASK_SQUARE[+Square::C2];
    EXPECT_EQ(MASK_KNIGHT_ATTACKS[+Square::A1], expected);

    // From H4: G6 F5 F3 G2
    expected = MASK_SQUARE[+Square::G6] | MASK_SQUARE[+Square::F5] |
               MASK_SQUARE[+Square::F3] | MASK_SQUARE[+Square::G2];
    EXPECT_EQ(MASK_KNIGHT_ATTACKS[+Square::H4], expected);

    // From E4 knight: D6 F6 C5 G5 C3 G3 D2 F2
    expected = MASK_SQUARE[+Square::D6] | MASK_SQUARE[+Square::F6] |
               MASK_SQUARE[+Square::C5] | MASK_SQUARE[+Square::G5] |
               MASK_SQUARE[+Square::C3] | MASK_SQUARE[+Square::G3] |
               MASK_SQUARE[+Square::D2] | MASK_SQUARE[+Square::F2];
    EXPECT_EQ(MASK_KNIGHT_ATTACKS[+Square::E4], expected);
}

TEST(BitboardTests, PawnAttacks) {
    // From E2 white pawn: D3 F3
    Bitboard expected = MASK_SQUARE[+Square::D3] | MASK_SQUARE[+Square::F3];
    EXPECT_EQ(MASK_PAWN_ATTACKS[+Color::White][+Square::E2], expected);

    // From A7 black pawn: B6
    expected = MASK_SQUARE[+Square::B6];
    EXPECT_EQ(MASK_PAWN_ATTACKS[+Color::Black][+Square::A7], expected);

    // From H4 black pawn: G3
    expected = MASK_SQUARE[+Square::G3];
    EXPECT_EQ(MASK_PAWN_ATTACKS[+Color::Black][+Square::H4], expected);
}

TEST(BitboardTests, RookAttacks) {
    // From D4 rook on empty board
    Bitboard expected = 0ULL;
    for (int r = 0; r < 8; r++) {
        if (r != 3) expected |= MASK_SQUARE[+create_square(3, r)];
    }
    for (int f = 0; f < 8; f++) {
        if (f != 3) expected |= MASK_SQUARE[+create_square(f, 3)];
    }
    EXPECT_EQ(MASK_ROOK_ATTACKS[+Square::D4], expected);
}

TEST(BitboardTests, BishopAttacks) {
    // From D4 bishop on empty board
    Bitboard expected = 0ULL;
    for (int i = 1; i < 8; i++) {
        // Up-Right
        int f = 3 + i;
        int r = 3 + i;
        if (f < 8 && r < 8) expected |= MASK_SQUARE[+create_square(f, r)];

        // Up-Left
        f = 3 - i;
        r = 3 + i;
        if (f >= 0 && r < 8) expected |= MASK_SQUARE[+create_square(f, r)];

        // Down-Right
        f = 3 + i;
        r = 3 - i;
        if (f < 8 && r >= 0) expected |= MASK_SQUARE[+create_square(f, r)];

        // Down-Left
        f = 3 - i;
        r = 3 - i;
        if (f >= 0 && r >= 0) expected |= MASK_SQUARE[+create_square(f, r)];
    }
    EXPECT_EQ(MASK_BISHOP_ATTACKS[+Square::D4], expected);
}


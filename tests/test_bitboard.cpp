#include "gtest/gtest.h"
#include "core/bitboard.hpp"

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

TEST(BitboardTests, LsbAndPopLsb) {
    Bitboard bb = MASK_SQUARE[+Square::E4] | MASK_SQUARE[+Square::A1];
    Square first = lsb(bb);
    EXPECT_EQ(first, Square::A1);
    pop_lsb(bb);
    EXPECT_EQ(lsb(bb), Square::E4);
    pop_lsb(bb);
    EXPECT_EQ(bb, 0ULL);
}

TEST(BitboardTests, KnightAttacks) {
    // From A1, knight attacks should be B3 and C2
    Bitboard expected = MASK_SQUARE[+Square::B3] | MASK_SQUARE[+Square::C2];
    EXPECT_EQ(MASK_KNIGHT_ATTACKS[+Square::A1], expected);
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


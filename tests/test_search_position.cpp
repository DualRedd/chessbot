#include "gtest/gtest.h"
#include "ai/search_position.hpp"

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

TEST(SearchPositionTests, InitialEval) {
    SearchPosition position;
    position.set_board(CHESS_START_POSITION);
    EXPECT_EQ(position.get_eval(), 0) << "Initial position should have eval 0.";
}

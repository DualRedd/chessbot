#include "gtest/gtest.h"
#include "engine/search_position.hpp"
#include "core/move_generation.hpp"

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

TEST(SearchPositionTests, InitialEval) {
    SearchPosition position;
    position.set_board(CHESS_START_POSITION);
    EXPECT_EQ(position.get_eval(), 0) << "Initial position should have eval 0.";
}


// Test make undo constistency
TEST(SearchPositionTests, MakeUndoConsistency) {
    const int move_count = 100;

    SearchPosition position;
    position.set_board(CHESS_START_POSITION);

    MoveList move_list;
    std::vector<int32_t> eval_history;
    eval_history.push_back(position.get_eval());

    for (int i = 0; i < move_count; ++i) {
        move_list.generate<GenerateType::Legal>(position.get_position());
        ASSERT_FALSE(move_list.count() == 0);
        int r = std::rand() % static_cast<int>(move_list.count());
        position.make_move(move_list[r]);
        eval_history.push_back(position.get_eval());
    }
    for (int i = move_count - 1; i >= 0; --i) {
        EXPECT_EQ(position.get_eval(), eval_history[i + 1]) << "Eval mismatch before undo at step " << (move_count - i);
        ASSERT_TRUE(position.undo_move()) << "Undo failed at step " << (move_count - i);
        EXPECT_EQ(position.get_eval(), eval_history[i]) << "Eval mismatch after undo at step " << (move_count - i);
    }
}

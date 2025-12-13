#include <random>

#include "gtest/gtest.h"
#include "engine/search_position.hpp"
#include "core/move_generation.hpp"
#include "positions.hpp"

static std::mt19937 rng;

TEST(SearchPositionTests, InitialEval) {
    SearchPosition ss;
    ss.set_board(CHESS_START_POSITION);
    EXPECT_EQ(ss.get_eval(), 0) << "Initial position should have eval 0.";
}

TEST(SearchPositionTests, MakeUndoConsistency) {
    rng.seed(42);

    SearchPosition ss, rebuilt;
    MoveList move_list;

    for (const FEN& fen : TEST_POSITIONS) {
        ss.set_board(fen);
        int32_t orig_eval = ss.get_eval();

        move_list.generate<GenerateType::Legal>(ss.get_position());
        ASSERT_FALSE(move_list.count() == 0) << "No legal moves in position: " << fen;
        
        for (Move move : move_list) {
            ss.make_move(move);

            // check that rebuilding from FEN reproduces the same eval
            int32_t eval1 = ss.get_eval();
            rebuilt.set_board(ss.get_position().to_fen()); // drop move history
            int32_t eval2 = rebuilt.get_eval();
            ASSERT_EQ(eval1, eval2) << "Eval mismatch after move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);

            // Check eval matches after undo
            ss.undo_move();
            int32_t eval3 = ss.get_eval();
            ASSERT_EQ(orig_eval, eval3) << "Eval mismatch after undo move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);
        }
    }
}

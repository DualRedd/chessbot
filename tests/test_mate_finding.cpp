#include "gtest/gtest.h"
#include "engine/minimax_engine.hpp"
#include "core/move_generation.hpp"
#include "positions.hpp"

// Helper function to test one case
static inline int test_case(const FEN& fen, int expected_mate_in, int64_t node_limit) {
    const bool enable_output = false;
    const size_t tt_size_megabytes = 64ULL;
    const double time_limit_seconds = 300.0; // node limit should be the main limiter
    const int max_depth = std::max(20, expected_mate_in * 2 + 4);

    auto engine = std::make_unique<MinimaxAI>(max_depth, time_limit_seconds, tt_size_megabytes, enable_output);
    engine->set_max_nodes(node_limit);
    engine->set_board(fen);

    auto[mate_distance, move] = engine->find_mate();
    return mate_distance;
}

TEST(MateFinding, MateIn1Positions) {
    const int64_t node_limit = 1000;
    for (const auto& fen : MATE_IN_1) {
        int mate_distance = test_case(fen, 1, node_limit);
        ASSERT_EQ(mate_distance, 1) << "FEN: " << fen << " expected mate in "
                                    << 1 << " but got " << mate_distance;
    }
}

TEST(MateFinding, MateIn2Positions) {
    const int64_t node_limit = 10'000;
    for (const auto& fen : MATE_IN_2) {
        int mate_distance = test_case(fen, 2, node_limit);
        ASSERT_EQ(mate_distance, 2) << "FEN: " << fen << " expected mate in "
                                    << 2 << " but got " << mate_distance;
    }
}

TEST(MateFinding, MateIn3Positions) {
    const int64_t node_limit = 100'000;
    for (const auto& fen : MATE_IN_3) {
        int mate_distance = test_case(fen, 3, node_limit);
        ASSERT_EQ(mate_distance, 3) << "FEN: " << fen << " expected mate in "
                                    << 3 << " but got " << mate_distance;
    }
}

TEST(MateFinding, MateIn4Positions) {
    const int64_t node_limit = 1'000'000;
    for (const auto& fen : MATE_IN_4) {
        int mate_distance = test_case(fen, 4, node_limit);
        ASSERT_EQ(mate_distance, 4) << "FEN: " << fen << " expected mate in "
                                    << 4 << " but got " << mate_distance;
    }
}

TEST(MateFinding, MateIn6Positions) {
    const int64_t node_limit = 10'000'000;
    for (const auto& fen : MATE_IN_6) {
        int mate_distance = test_case(fen, 6, node_limit);
        ASSERT_EQ(mate_distance, 6) << "FEN: " << fen << " expected mate in "
                                    << 6 << " but got " << mate_distance;
    }
}


#include "gtest/gtest.h"
#include "core/bitboard.hpp"

#include <functional>

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

TEST(BitboardTests, Perft) {
    Board board;
    std::function<uint64_t(int)> perft = [&perft, &board](int depth) {
        if(depth == 0)
            return uint64_t(1);

        uint64_t node_count = 0;
        auto moves = board.generate_legal_moves();

        if(depth == 1)
            return uint64_t(moves.size());

        for(const Move& move : moves) {
            board.make_move(move);
            node_count += perft(depth - 1);
            board.undo_move();
        }

        return node_count;
    };

    // Positions and results from https://www.chessprogramming.org/Perft_Results#Test_Suites
    board.set_from_fen(CHESS_START_POSITION);
    EXPECT_EQ(perft(3), 8902);

    board.set_from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ");
    EXPECT_EQ(perft(4), 43238);

    board.set_from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(perft(4), 422333);

    board.set_from_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(perft(3), 62379);

    board.set_from_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(perft(3), 89890);
}

TEST(BitboardTests, MoveFromUCI) {
    // TODO: test more diverse positions
    Board board(CHESS_START_POSITION);
    auto moves = board.generate_legal_moves();
    for(const Move& move : moves) {
        UCI uci_move = MoveEncoding::to_uci(move);
        EXPECT_EQ(board.move_from_uci(uci_move), move);
    }
}

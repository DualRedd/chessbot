#include "gtest/gtest.h"
#include "core/bitboard.hpp"

#include <functional>

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

TEST(BitboardTests, FromValidLegalFEN) {
    Board board;
    auto test_no_throw = [&board] (const FEN& fen, bool allow_illegal) {
        EXPECT_NO_THROW(board.set_from_fen(fen, allow_illegal)) << "FEN: " << fen;
    };
    
    test_no_throw("k1K5/8/8/4pP2/8/8/8/8", false); // Only board
    test_no_throw("k1K5/8/8/4pP2/8/8/8/8 w - e6", false); // Valid white en passant
    test_no_throw("k1K5/8/8/8/4pP2/8/8/8 b - f3", false); // Valid black en passant
    test_no_throw("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", false);
}

TEST(BitboardTests, FromValidIllegalFEN) {
    Board board;
    auto test_illegal_but_valid = [&board] (const FEN& fen) {
        EXPECT_THROW(board.set_from_fen(fen, false), std::invalid_argument) << "FEN: " << fen;
        EXPECT_NO_THROW(board.set_from_fen(fen, true)) << "FEN: " << fen;
    };

    // Illegal (unreachable but valid) positions
    test_illegal_but_valid("8/8/8/8/8/8/8/8"); // No kings
    test_illegal_but_valid("8/8/8/8/K7/8/8/8"); // Only white king
    test_illegal_but_valid("8/8/8/8/k7/8/8/8"); // Only black king
    test_illegal_but_valid("8/KKK5/8/8/kkk5/8/8/7K"); // Multiple kings
    test_illegal_but_valid("8/8/8/8/kK6/8/8/8"); // Both kings in check
    test_illegal_but_valid("k1K4p/8/8/8/8/8/8/8"); // Pawn on rank 8
    test_illegal_but_valid("k1K5/8/8/8/8/8/8/7P"); // Pawn on rank 1
}

TEST(BitboardTests, FromInvalidFEN) {
    Board board;
    auto test_throw = [&board] (const FEN& fen) {
        EXPECT_THROW(board.set_from_fen(fen), std::invalid_argument) << "FEN: " << fen;
    };
    
    // Board
    test_throw(""); // Empty string
    test_throw("k1K5/8/8/8/8/8/8"); // Missing rank
    test_throw("k1K5/8/8/8/8/8/8/5"); // Rank missing files
    test_throw("k1K5/8/8/5/8/8/8/8"); // Rank missing files
    test_throw("k1K5/8/8/8/8/8/8/8k"); // Rank extra files
    test_throw("k1K5/8/8/8k/8/8/8/8"); // Rank extra files
    test_throw("k1K5/8/8/8/8/8/8/9"); // Invalid empty space count
    test_throw("k1K5/8/8/9/8/8/8/8"); // Invalid empty space count
    test_throw("k1K5/8/8/7t/8/8/8/8"); // Invalid piece char

    // Side to move
    test_throw("k1K5/8/8/8/8/8/8/8 t"); // Invalid side to move char

    // Castling
    test_throw("4k3/8/8/8/8/8/8/4K3 w KQkqqq"); // Too many chars
    test_throw("4k3/8/8/8/8/8/8/4K3 w ABab"); // Unknown char
    for(char c : {'K', 'Q', 'k', 'q'}){
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 w ") + c); // Missing rook for castling
        test_throw(std::string("r2k3R/8/8/8/8/8/8/r2K3R w ") + c); // Missing king for castling
    }

    // En passant
    for(char s : {'w', 'b'}){
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 ") + s + " - e3x"); // Too many chars
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 ") + s + " - e"); // Too few chars
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 ") + s + " - j6"); // Invalid file
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 ") + s + " - X6"); // Invalid file
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 ") + s + " - a1"); // Invalid rank
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 ") + s + " - a1"); // Invalid rank
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 ") + s + " - a6"); // Missing pawn
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 ") + s + " - a3"); // Missing pawn
    }

    // Move counters
    test_throw("4k3/8/8/8/8/8/8/4K3 b - - a 1"); // Invalid halfmove count
    test_throw("4k3/8/8/8/8/8/8/4K3 b - - 1 b"); // Invalid fullmove count
}


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

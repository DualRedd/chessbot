#include <functional>
#include <random>

#include "gtest/gtest.h"
#include "core/position.hpp"
#include "core/move_generation.hpp"
#include "positions.hpp"

static std::mt19937 rng;

TEST(PositionTests, CopyConstructor) {
    Position original(CHESS_START_POSITION);
    Position copy(original, false); // do not copy history
    EXPECT_EQ(copy.to_fen(), CHESS_START_POSITION) << "the copy does not return the same FEN as the original was set to.";
}

TEST(PositionTests, CopyConstructorWithHistory) {
    rng.seed(42);

    // Make random moves on a position
    const int moves = 100;
    MoveList move_list;
    Position original(CHESS_START_POSITION);
    for (int i = 0; i < moves; i++) {
        move_list.generate<GenerateType::Legal>(original);
        ASSERT_TRUE(move_list.count() > 0);
        int r = rng() % static_cast<int>(move_list.count());
        original.make_move(move_list[r]);
    }

    // Make a copy and test undoing moves
    Position copy(original, true);
    for (int i = 0; i < moves; i++) {
        EXPECT_TRUE(copy.undo_move());
    }
    EXPECT_EQ(copy.to_fen(), CHESS_START_POSITION)
        << "After doing moves on original position, copying the position and undoing moves on the copy, the copy does not return the same FEN as the original was set to.";
}

TEST(PositionTests, FromValidLegalFEN) {
    Position position;
    auto test_no_throw = [&position](const FEN& fen) {
        EXPECT_NO_THROW(position.from_fen(fen)) << "Case: " << fen;
    };

    test_no_throw("k1K5/8/8/4pP2/8/8/8/8"); // Only board
    test_no_throw("k1K5/8/8/4pP2/8/8/8/8 b"); // Valid default side to move
    test_no_throw("r3k2r/8/8/4pP2/8/8/8/1R2K2R b kKq"); // Valid castling rights
    test_no_throw("k1K5/8/8/4pP2/8/8/8/8 w - e6"); // Valid white en passant
    test_no_throw("k1K5/8/8/8/4pP2/8/8/8 b - f3"); // Valid black en passant
    test_no_throw("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"); // Complex position
}

TEST(PositionTests, FromValidIllegalFEN) {
    Position position;
    auto test_illegal = [&position](const FEN& fen) {
        EXPECT_THROW(position.from_fen(fen), std::invalid_argument) << "Case: " << fen;
    };

    // Illegal (unreachable) positions
    test_illegal("8/8/8/8/8/8/8/8"); // No kings
    test_illegal("8/8/8/8/K7/8/8/8"); // Only white king
    test_illegal("8/8/8/8/k7/8/8/8"); // Only black king
    test_illegal("8/KKK5/8/8/kkk5/8/8/7K"); // Multiple kings
    test_illegal("8/8/8/8/kK6/8/8/8"); // Both kings in check
    test_illegal("k1K4p/8/8/8/8/8/8/8"); // Pawn on rank 8
    test_illegal("k1K5/8/8/8/8/8/8/7P"); // Pawn on rank 1
}

TEST(PositionTests, FromInvalidFEN) {
    Position position;
    auto test_throw = [&position](const FEN& fen) {
        EXPECT_THROW(position.from_fen(fen), std::invalid_argument) << "Case: " << fen;
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
    for (char c : {'K', 'Q', 'k', 'q'}) {
        test_throw(std::string("4k3/8/8/8/8/8/8/4K3 w ") + c); // Missing rook for castling
        test_throw(std::string("r2k3R/8/8/8/8/8/8/r2K3R w ") + c); // Missing king for castling
    }

    // En passant
    for (char s : {'w', 'b'}) {
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

TEST(PositionTests, ToFEN) {
    Position position;
    auto test_fen = [&position](const FEN& fen) {
        EXPECT_NO_THROW(position.from_fen(fen)) << "Case: " << fen;
        EXPECT_EQ(position.to_fen(), fen) << "Case: " << fen;
    };

    test_fen("rnbqkbnr/8/8/8/8/8/8/RNBQKBNR b KQkq - 1 100");
    test_fen("k1K5/8/8/4pP2/8/8/8/8 w - e6 5 10");
    test_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
}

TEST(PositionTests, MoveFromUCI) {
    Position position(COMPLEX_POSITION);
    EXPECT_THROW(position.move_from_uci("aa"), std::invalid_argument) << "Case: UCI too short";
    EXPECT_THROW(position.move_from_uci("e5e6e7"), std::invalid_argument) << "Case: UCI too long";
    EXPECT_THROW(position.move_from_uci("e5e6k"), std::invalid_argument) << "Case: UCI invalid promotion";
    EXPECT_THROW(position.move_from_uci("A5e6"), std::invalid_argument) << "Case: UCI invalid file";
    EXPECT_THROW(position.move_from_uci("a5A6"), std::invalid_argument) << "Case: UCI invalid file";
    EXPECT_THROW(position.move_from_uci("a0a6"), std::invalid_argument) << "Case: UCI invalid rank";
    EXPECT_THROW(position.move_from_uci("a5a0"), std::invalid_argument) << "Case: UCI invalid rank";

    MoveList move_list;
    auto test_fen = [&position, &move_list](const FEN& fen) {
        position.from_fen(fen);
        move_list.generate<GenerateType::Legal>(position);
        for (const Move& move : move_list) {
            EXPECT_EQ(position.move_from_uci(MoveEncoding::to_uci(move)), move) << "Case: Move to UCI back to Move";
        }
    };

    test_fen(CHESS_START_POSITION);
    test_fen("r3k2r/1Pp2ppp/1b3nbN/nP1pP3/BBP5/q4N2/Pp1P1P1P/R3K2R w KQkq d6 0 3"); // en passant, castling, promo
}

TEST(PositionTests, GetLastMove) {
    rng.seed(42);

    // Make random moves on a board
    const int moves = 50;
    MoveList move_list;
    Position position(COMPLEX_POSITION);
    EXPECT_EQ(position.get_last_move(), std::nullopt) << "should return std::nullopt with no moves made.";
    for (int i = 0; i < moves; i++) {
        move_list.generate<GenerateType::Legal>(position);
        ASSERT_TRUE(move_list.count() > 0);
        int r = rng() % static_cast<int>(move_list.count());
        position.make_move(move_list[r]);
        EXPECT_EQ(position.get_last_move(), move_list[r]) << "last move does not match the move just made.";
    }
    for (int i = 0; i < moves; i++) {
        position.undo_move();
    }
    EXPECT_EQ(position.get_last_move(), std::nullopt) << "should return std::nullopt after undoing all moves.";
}

TEST(PositionTests, PinnersAndBlockersMatchBoardState) {
    Position position("r4B1K/4n1r1/5b2/3N4/2P5/R2r3k/8/8 b - - 0 1");
    ASSERT_EQ(position.get_pinners(Color::White), MASK_SQUARE[+Square::A8]);
    ASSERT_EQ(position.get_pinners(Color::Black), MASK_SQUARE[+Square::A3]);
    ASSERT_EQ(position.get_king_blockers(Color::White), MASK_SQUARE[+Square::F8] | MASK_SQUARE[+Square::G7]);
    ASSERT_EQ(position.get_king_blockers(Color::Black), MASK_SQUARE[+Square::D3]);
}

TEST(PositionTests, GivesCheckMatchesInCheckAfterMove) {
    MoveList move_list;
    Position position;

    for (const FEN& fen : TEST_POSITIONS) {
        position.from_fen(fen);
        move_list.generate<GenerateType::Legal>(position);
        for (const Move& move : move_list) {
            bool gives_check = position.gives_check(move);
            position.make_move(move);
            bool in_check = position.in_check(position.get_side_to_move());
            ASSERT_TRUE(position.undo_move());
            ASSERT_EQ(gives_check, in_check) << "gives_check() result does not match actual check state after move. FEN: "
                                            << fen << ", Move: " << MoveEncoding::to_uci(move);
        }
    }
}

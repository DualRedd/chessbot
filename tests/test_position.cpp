#include "gtest/gtest.h"

#include "core/position.hpp"
#include "core/move_generation.hpp"
#include <functional>

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

TEST(BoardCopyTests, CopyConstructor) {
    // Make random moves on a board
    const int moves = 100;
    Position original(CHESS_START_POSITION);
    Position copy(original, false); // do not copy history
    EXPECT_EQ(copy.to_fen(), CHESS_START_POSITION) << "the copy does not return the same FEN as the original was set to.";
}

TEST(BoardCopyTests, CopyConstructorWithHistory) {
    // Make random moves on a board
    const int moves = 100;
    MoveList move_list;
    Position original(CHESS_START_POSITION);
    for (int i = 0; i < moves; i++) {
        move_list.generate_legal(original);
        ASSERT_TRUE(move_list.count() > 0);
        int r = std::rand() % static_cast<int>(move_list.count());
        original.make_move(move_list[r]);
    }

    // Make a copy and test that undoing moves works
    Position copy(original, true);// do not copy history
    for (int i = 0; i < moves; i++) {
        EXPECT_TRUE(copy.undo_move());
    }
    EXPECT_EQ(copy.to_fen(), CHESS_START_POSITION)
        << "After doing moves on original board, copying the board and undoing moves on the copy, the copy does not return the same FEN as the original was set to.";
}

TEST(BitboardTests, FromValidLegalFEN) {
    Position board;
    auto test_no_throw = [&board](const FEN& fen, bool allow_illegal) {
        EXPECT_NO_THROW(board.from_fen(fen, allow_illegal)) << "Case: " << fen;
    };

    test_no_throw("k1K5/8/8/4pP2/8/8/8/8", false); // Only board
    test_no_throw("k1K5/8/8/4pP2/8/8/8/8 w - e6", false); // Valid white en passant
    test_no_throw("k1K5/8/8/8/4pP2/8/8/8 b - f3", false); // Valid black en passant
    test_no_throw("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", false);
}

TEST(BitboardTests, FromValidIllegalFEN) {
    Position board;
    auto test_illegal_but_valid = [&board](const FEN& fen) {
        EXPECT_THROW(board.from_fen(fen, false), std::invalid_argument) << "Case: " << fen;
        EXPECT_NO_THROW(board.from_fen(fen, true)) << "Case: " << fen;
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
    Position board;
    auto test_throw = [&board](const FEN& fen) {
        EXPECT_THROW(board.from_fen(fen), std::invalid_argument) << "Case: " << fen;
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

TEST(BitboardTests, ToFEN) {
    Position board;
    auto test_fen = [&board](const FEN& fen) {
        EXPECT_NO_THROW(board.from_fen(fen)) << "Case: " << fen;
        EXPECT_EQ(board.to_fen(), fen) << "Case: " << fen;
    };

    test_fen("rnbqkbnr/8/8/8/8/8/8/RNBQKBNR b KQkq - 1 100");
    test_fen("k1K5/8/8/4pP2/8/8/8/8 w - e6 5 10");
    test_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
}

TEST(BitboardTests, SideToMove) {
    Position board;
    board.from_fen("8/8/8/8/6Nn/8/8/r7 b", true);
    EXPECT_EQ(board.get_side_to_move(), Color::Black) << "Case: correct after intial set from FEN.";
    board.make_move(MoveEncoding::encode<MoveType::Normal>(Square::A1, Square::A8));
    EXPECT_EQ(board.get_side_to_move(), Color::White) << "Case: correct after a move was made.";
}

TEST(BitboardTests, MoveFromUCI) {
    Position board(CHESS_START_POSITION);
    EXPECT_THROW(board.move_from_uci("aa"), std::invalid_argument) << "Case: UCI too short";
    EXPECT_THROW(board.move_from_uci("e5e6e7"), std::invalid_argument) << "Case: UCI too long";
    EXPECT_THROW(board.move_from_uci("e5e6k"), std::invalid_argument) << "Case: UCI invalid promotion";
    EXPECT_THROW(board.move_from_uci("A5e6"), std::invalid_argument) << "Case: UCI invalid file";
    EXPECT_THROW(board.move_from_uci("a5A6"), std::invalid_argument) << "Case: UCI invalid file";
    EXPECT_THROW(board.move_from_uci("a0a6"), std::invalid_argument) << "Case: UCI invalid rank";
    EXPECT_THROW(board.move_from_uci("a5a0"), std::invalid_argument) << "Case: UCI invalid rank";

    MoveList move_list;
    auto test_fen = [&board, &move_list](const FEN& fen) {
        board.from_fen(fen);
        move_list.generate_legal(board);
        for (const Move& move : move_list) {
            EXPECT_EQ(board.move_from_uci(MoveEncoding::to_uci(move)), move) << "Case: Move to UCI back to Move";
        }
    };

    test_fen(CHESS_START_POSITION);
    test_fen("r3k2r/1Pp2ppp/1b3nbN/nP1pP3/BBP5/q4N2/Pp1P1P1P/R3K2R w KQkq d6 0 3"); // en passant, castling, promo
}

TEST(BitboardTests, GetLastMove) {
    // Make random moves on a board
    const int moves = 50;
    MoveList move_list;
    Position board(CHESS_START_POSITION);
    EXPECT_EQ(board.get_last_move(), std::nullopt) << "should return std::nullopt with no moves made.";
    for (int i = 0; i < moves; i++) {
        move_list.generate_legal(board);
        ASSERT_FALSE(move_list.count() == 0);
        int r = std::rand() % static_cast<int>(move_list.count());
        board.make_move(move_list[r]);
        EXPECT_EQ(board.get_last_move(), move_list[r]) << "last move does not match the move just made.";
    }
    for (int i = 0; i < moves; i++) {
        board.undo_move();
    }
    EXPECT_EQ(board.get_last_move(), std::nullopt) << "should return std::nullopt after undoing all moves.";
}


TEST(BitboardTests, Perft) {
    Position board;
    std::function<uint64_t(int)> perft = [&perft, &board](int depth) {
        if (depth == 0)
            return uint64_t(1);
            
        uint64_t node_count = 0;
        MoveList move_list;
        move_list.generate_legal(board);

        if (depth == 1)
           return uint64_t(move_list.count());

        for (const Move& move : move_list) {
            board.make_move(move);
            EXPECT_FALSE(board.in_check(opponent(board.get_side_to_move())));
            node_count += perft(depth - 1);
            EXPECT_TRUE(board.undo_move());
        }

        return node_count;
    };

    // Positions and results from https://www.chessprogramming.org/Perft_Results#Test_Suites
    board.from_fen(CHESS_START_POSITION);
    EXPECT_EQ(perft(3), 8902);

    board.from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ");
    EXPECT_EQ(perft(4), 43238);

    board.from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(perft(4), 422333);

    board.from_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(perft(3), 62379);

    board.from_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(perft(3), 89890);
}












TEST(BitboardTest, ShiftRightLeftNoWrap) {
    // Basic horizontal shifts and edge wrap prevention
    EXPECT_EQ(shift_bb<Shift::Right>(MASK_SQUARE[+Square::A1]), MASK_SQUARE[+Square::B1]);
    EXPECT_EQ(shift_bb<Shift::Left>(MASK_SQUARE[+Square::B1]), MASK_SQUARE[+Square::A1]);

    // Shifts off the board must produce zero (no wrap)
    EXPECT_EQ(shift_bb<Shift::Left>(MASK_SQUARE[+Square::A1]), 0ULL);
    EXPECT_EQ(shift_bb<Shift::Right>(MASK_SQUARE[+Square::H1]), 0ULL);
}

TEST(BitboardTest, DiagonalAndVerticalShifts) {
    // Simple diagonal / vertical correctness
    EXPECT_EQ(shift_bb<Shift::Up>(MASK_SQUARE[+Square::A1]), MASK_SQUARE[+Square::A2]);
    EXPECT_EQ(shift_bb<Shift::UpRight>(MASK_SQUARE[+Square::A1]), MASK_SQUARE[+Square::B2]);
    EXPECT_EQ(shift_bb<Shift::UpLeft>(MASK_SQUARE[+Square::H1]), MASK_SQUARE[+Square::G2]);

    // Double shifts
    EXPECT_EQ(shift_bb<Shift::DoubleUp>(MASK_SQUARE[+Square::A1]), MASK_SQUARE[+Square::A3]);
    EXPECT_EQ(shift_bb<Shift::DoubleDown>(MASK_SQUARE[+Square::A3]), MASK_SQUARE[+Square::A1]);
}

TEST(BitboardTest, LsbAndPopLsb) {
    Bitboard bb = MASK_SQUARE[+Square::E4] | MASK_SQUARE[+Square::A1];
    Square first = lsb(bb);
    EXPECT_EQ(first, Square::A1);
    pop_lsb(bb);
    EXPECT_EQ(lsb(bb), Square::E4);

    // pop all leaves zero
    pop_lsb(bb);
    EXPECT_EQ(bb, 0ULL);
}

TEST(BitboardTest, KnightAttacksCorner) {
    // From A1, knight attacks should be B3 and C2
    Bitboard expected = MASK_SQUARE[+Square::B3] | MASK_SQUARE[+Square::C2];
    EXPECT_EQ(MASK_KNIGHT_ATTACKS[+Square::A1], expected);
}

TEST(BitboardTest, KingAttacks) {
    // From E1 king: D1 F1 D2 E2 F2
    Bitboard expected =
        MASK_SQUARE[+Square::D1] | MASK_SQUARE[+Square::F1] |
        MASK_SQUARE[+Square::D2] | MASK_SQUARE[+Square::E2] |
        MASK_SQUARE[+Square::F2];
    EXPECT_EQ(MASK_KING_ATTACKS[+Square::E1], expected);

    // From A1 king: A2 B2 B1
    expected =
        MASK_SQUARE[+Square::A2] | MASK_SQUARE[+Square::B2] |
        MASK_SQUARE[+Square::B1];
    EXPECT_EQ(MASK_KING_ATTACKS[+Square::A1], expected);
}

TEST(BitboardTest, SlidingAttacksStopsAtBlocker) {
    // Rook on A1 with a blocker at A3: should see A2 and A3 only upward
    Bitboard occ = MASK_SQUARE[+Square::A3];
    Bitboard attacks = sliding_attacks<Shift::Up>(Square::A1, occ);
    Bitboard expected = MASK_SQUARE[+Square::A2] | MASK_SQUARE[+Square::A3];
    EXPECT_EQ(attacks, expected);

    // Bishop direction: from C1 with blocker at E3 should include D2 and E3 on that diagonal
    occ = MASK_SQUARE[+Square::E3];
    attacks = sliding_attacks<Shift::UpRight>(Square::C1, occ);
    expected = MASK_SQUARE[+Square::D2] | MASK_SQUARE[+Square::E3];
    EXPECT_EQ(attacks, expected);
}


#include "gtest/gtest.h"
#include "core/position.hpp"
#include "core/move_generation.hpp"

#include <functional>
#include <random>
#include <set>

static std::mt19937 rng;

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

const std::string TEST_POSITIONS[] = {
    CHESS_START_POSITION,
    "rnbqk2r/p3ppPp/5n2/Pp1p4/8/3P1Q2/1pP2PPP/R1B1K2R w KQkq b6 0 1", // ep, castling, promo
    "3k4/8/8/8/8/5q2/8/3r2K1 w - - 0 1" // check
    "3k4/8/8/8/1N1K1q2/8/2B1PN2/3r4 w - - 0 1" // double check
    "rnbqkbnr/pppp1ppp/8/4Q3/3P4/8/PPP1PPPP/RNB1KBNR b KQkq - 0 2",
    "2r4r/p5pp/8/2B1Q2k/1P1P4/2P3P1/P4P1P/4K2R b K - 0 2",
    "2r4r/pB4pp/2b5/3Q4/1P6/2P2kP1/P4P1P/4K2R b K - 0 2",
    "7r/1R1p1kpp/8/4P3/2B5/1P4P1/P6P/3QK3 b - - 0 2",
    "7r/5kpp/8/3pP3/1R2K3/1P4P1/P4B1P/3Q4 w - d6 0 2",
    "b7/8/4k3/3pP3/4K3/8/8/8 w - d6 0 2",
};


TEST(MoveGenerationTests, Perft) {
    Position position;
    std::function<uint64_t(int)> perft = [&perft, &position](int depth) {
        if (depth == 0)
            return uint64_t(1);
            
        uint64_t node_count = 0;
        MoveList move_list;
        move_list.generate<GenerateType::Legal>(position);

        if (depth == 1)
           return uint64_t(move_list.count());

        for (const Move& move : move_list) {
            position.make_move(move);
            EXPECT_FALSE(position.in_check(opponent(position.get_side_to_move())))
                << "Generated illegal move '" << MoveEncoding::to_uci(move)
                <<  "' in perft. Board FEN: " << position.to_fen();
            node_count += perft(depth - 1);
            EXPECT_TRUE(position.undo_move()) << "Failed to undo move in perft.";
        }

        return node_count;
    };

    // Positions and results from https://www.chessprogramming.org/Perft_Results#Test_Suites
    position.from_fen(CHESS_START_POSITION);
    EXPECT_EQ(perft(4), 197281);

    position.from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ");
    EXPECT_EQ(perft(4), 43238);

    position.from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(perft(4), 422333);

    position.from_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(perft(4), 2103487);

    position.from_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(perft(4), 3894594);
}

TEST(MoveGenerationTests, CapturePlusQuietEqualsLegal) {
    rng.seed(42);

    MoveList all_moves, capture_moves, quiet_moves;
    Position position;

    for (const FEN& fen : TEST_POSITIONS) {
        position.from_fen(fen);
        if (position.in_check()) {
            // Skip positions in check (capture and quiet generation not valid)
            continue;
        }

        // Test equality
        all_moves.generate<GenerateType::Legal>(position);
        ASSERT_TRUE(all_moves.count() > 0);
        capture_moves.generate<GenerateType::Captures>(position);
        quiet_moves.generate<GenerateType::Quiets>(position);

        for (const Move& move : all_moves) {
            bool in_captures = false;
            for (size_t k = 0; k < capture_moves.count(); k++) {
                if (move == capture_moves[k]) {
                    in_captures = true;
                    break;
                }
            }
            bool in_quiets = false;
            for (size_t k = 0; k < quiet_moves.count(); k++) {
                if (move == quiet_moves[k]) {
                    in_quiets = true;
                    break;
                }
            }
            EXPECT_TRUE(in_captures || in_quiets)
                << "Legal move " << MoveEncoding::to_uci(move)
                << " not found in captures or quiets.";
            EXPECT_FALSE(in_captures && in_quiets)
                << "Legal move " << MoveEncoding::to_uci(move)
                << " found in both captures and quiets.";
        }
    }
}

TEST(MoveGenerationTests, EvasionsDontLeaveInCheck) {
    rng.seed(50);

    MoveList evasion_moves;
    Position position;

    for (const FEN& fen : TEST_POSITIONS) {
        position.from_fen(fen);
        if (!position.in_check()) {
            // Skip positions not in check (evasion generation not valid)
            continue;
        }
        
        evasion_moves.generate<GenerateType::Evasions>(position);
        for (const Move& move : evasion_moves) {
            position.make_move(move);
            EXPECT_FALSE(position.in_check(opponent(position.get_side_to_move())))
                << "Evasion move '" << MoveEncoding::to_uci(move)
                <<  "' leaves king in check. Board FEN: " << position.to_fen();
            EXPECT_TRUE(position.undo_move()) << "Failed to undo move in evasion test.";
        }
    }
}


TEST(MoveGenerationTests, TestLegalityFunction) {
    rng.seed(99);

    MoveList moves;
    Position position;

    auto test_move = [&](Move move) {
        for (size_t k = 0; k < moves.count(); k++) {
            if (move == moves[k]) return;
        }
        ASSERT_FALSE(test_legality(position, move))
                    << "Move legality mismatch for move " << MoveEncoding::to_uci(move)
                    << ". Position FEN: " << position.to_fen()
                    << " Move type: " << (int)MoveEncoding::move_type(move)
                    << " From: " << (int)MoveEncoding::from_sq(move)
                    << " To: " << (int)MoveEncoding::to_sq(move)
                    << " Promoted to: " << (int)MoveEncoding::promo(move);
    };

    for (const FEN& fen : TEST_POSITIONS) {
        position.from_fen(fen);
        moves.generate<GenerateType::Legal>(position);
        ASSERT_TRUE(moves.count() > 0);

        // Test all possible illegal moves
        for(Square from = Square::A1; from < Square::H8; ++from) {
            for(Square to = Square::A1; to < Square::H8; ++to) {
                test_move(MoveEncoding::encode<MoveType::Normal>(from, to));
                test_move(MoveEncoding::encode<MoveType::Castle>(from, to));
                test_move(MoveEncoding::encode<MoveType::EnPassant>(from, to));
                for (PieceType promo : {PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen}) {
                    test_move(MoveEncoding::encode<MoveType::Promotion>(from, to, promo));
                }
            }
        }

        int r = rng() % static_cast<int>(moves.count());
        position.make_move(moves[r]);
    }
}



#include "gtest/gtest.h"

#include "core/position.hpp"
#include "engine/see.hpp"
#include "engine/value_tables.hpp"

TEST(StaticExchangeEvaluationTests, UndefendedPiece) {
    // white queen on d2 can capture black pawn on d5 (undefended).
    Position pos("4k3/8/8/3p4/8/8/3Q4/4K3 w - - 0 1");
    Move m = MoveEncoding::encode<MoveType::Normal>(Square::D2, Square::D5);
    ASSERT_TRUE(static_exchange_evaluation(pos, m, 0));
    ASSERT_TRUE(static_exchange_evaluation(pos, m, PIECE_VALUES[+PieceType::Pawn]));
    ASSERT_FALSE(static_exchange_evaluation(pos, m, PIECE_VALUES[+PieceType::Pawn] + 1));
}

TEST(StaticExchangeEvaluationTests, SingleDefender) {
    // queen capture: pawn on d5 is defended by black queen on d6
    Position pos("4k3/8/3q4/3p4/8/8/3Q4/4K3 w - - 0 1");
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("d2d5"), 0));

    // queen capture: pawn on d5 is defended by black bishop on g8
    pos.from_fen("4k1b1/8/8/3p4/8/8/3Q4/4K3 w - - 0 1"); 
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("d2d5"), 0));

    // knight trade on d5
    pos.from_fen("4k3/8/8/3n2r1/8/4N3/8/4K3 w - - 0 1"); 
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), 0));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), 1));
}

TEST(StaticExchangeEvaluationTests, MultipleDefenders) {
    // knight trade on d5: can stand-pat after equal exchange
    Position pos("4k1b1/8/8/3n2r1/8/4N3/8/3RK3 w - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), 0));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), 1));

    // multiple trades on d5
    pos.from_fen("3rk3/8/8/3n2r1/8/4N3/8/3RK3 w - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), 0));
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("d1d5"), 0));
}

TEST(StaticExchangeEvaluationTests, DiscoveredAttacker) {
    // can win pawn on d5 due to discovered attack after pawn recapture
    Position pos("4k3/1B6/2p5/3n2r1/8/4N3/8/3RK3 w - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), PIECE_VALUES[+PieceType::Pawn]));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), PIECE_VALUES[+PieceType::Pawn] + 1));

    // more defenders but still can win the pawn
    pos.from_fen("B3k1b1/1B6/2p5/3n2r1/8/4N3/8/3RK3 w - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), PIECE_VALUES[+PieceType::Pawn]));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e3d5"), PIECE_VALUES[+PieceType::Pawn] + 1));

    // rook discovered attack, can win a knight (or rook if white recaptures, but knight is less material)
    pos.from_fen("4k3/4n3/8/3N1Rr1/8/8/8/4K3 b - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), PIECE_VALUES[+PieceType::Knight]));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), PIECE_VALUES[+PieceType::Knight] + 1));
}

TEST(StaticExchangeEvaluationTests, KingInvolved) {
    // Equal exhange on d5, king recapture last move
    Position pos("4k3/4n3/8/3N1Rr1/2K5/8/8/8 b - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), 0));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), 1));

    // One more attacker, now king cannot recapture. Black wins a kinght at least (rook if white recaptures).
    pos.from_fen("4k3/4n3/8/3N1Rrr/2K5/8/8/8 b - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), PIECE_VALUES[+PieceType::Knight]));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), PIECE_VALUES[+PieceType::Knight] + 1));
}

TEST(StaticExchangeEvaluationTests, PinnedDefender) {
    // cannot win a pawn, as the black rook is pinned, only equal trade possible
    Position pos("7K/4n3/8/3N4/2P5/R2r3k/8/8 b - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), 0));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), 1));

    // Black can win a pawn. if white recaptures with the queen, the rook is then unpinned and can recapture also
    pos.from_fen("3q4/4n3/8/3N4/2P5/1Q1r3k/8/7K b - - 0 1");
    ASSERT_TRUE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), PIECE_VALUES[+PieceType::Pawn]));
    ASSERT_FALSE(static_exchange_evaluation(pos, pos.move_from_uci("e7d5"), PIECE_VALUES[+PieceType::Pawn] + 1));
}

#include "gtest/gtest.h"
#include "core/position.hpp"
#include "core/move_generation.hpp"

#include <unordered_map>
#include <random>

static std::mt19937 rng;

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const FEN COMPLEX_POSITION = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

// Helper: rebuild position from its FEN and return the zobrist hash
static uint64_t rebuild_hash_from_fen(const Position &position) {
    Position rebuilt(position.to_fen());
    return rebuilt.get_zobrist_hash();
}

TEST(ZobristTests, RebuildFromFENMatches) {
    rng.seed(42);
    Position position(CHESS_START_POSITION);
    const int moves = 500;

    // play random legal moves
    MoveList move_list;
    for (int i = 0; i < moves; ++i) {
        move_list.generate<GenerateType::Legal>(position);
        ASSERT_FALSE(move_list.count() == 0);
        int r = rng() % static_cast<int>(move_list.count());
        position.make_move(move_list[r]);

        // check that rebuilding from FEN reproduces the same zobrist
        uint64_t z1 = position.get_zobrist_hash();
        uint64_t z2 = rebuild_hash_from_fen(position);
        EXPECT_EQ(z1, z2) << "Mismatch after random moves" << i;
    }
}

TEST(ZobristTests, IncrementalMakeUndoConsistency) {
    rng.seed(42);
    Position position(CHESS_START_POSITION);
    const int moves = 100;
    std::vector<uint64_t> saved_hashes;
    saved_hashes.push_back(position.get_zobrist_hash());

    // play random moves and store hashes after each make
    MoveList move_list;
    for (int i = 0; i < moves; ++i) {
        move_list.generate<GenerateType::Legal>(position);
        ASSERT_FALSE(move_list.count() == 0);
        int r = rng() % static_cast<int>(move_list.count());
        position.make_move(move_list[r]);
        saved_hashes.push_back(position.get_zobrist_hash());
    }

    // undo moves one by one and verify the zobrist restores to saved values
    for (int i = 0; i < moves; ++i) {
        uint64_t expected = saved_hashes[saved_hashes.size() - 2 - i];
        ASSERT_TRUE(position.undo_move());
        uint64_t current = position.get_zobrist_hash();
        EXPECT_EQ(current, expected) << "Zobrist mismatch after undo at step " << i;
    }

    // after all undos we should be back to initial
    EXPECT_EQ(position.get_zobrist_hash(), saved_hashes.front());
}

TEST(ZobristTests, SpecialMoves) {
    Position position;
    MoveList move_list;
    auto is_legal_move = [&position, &move_list](const Move& move) {
        move_list.generate<GenerateType::Legal>(position);
        return std::find(move_list.begin(), move_list.end(), move) != move_list.end();
    };

    // castling position
    position.from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    Move move = position.move_from_uci("e1g1"); // white kingside castle
    ASSERT_TRUE(is_legal_move(move));
    position.make_move(move);
    uint64_t z = position.get_zobrist_hash();
    EXPECT_EQ(z, rebuild_hash_from_fen(position)) << "Castling zobrist value mismatch after make";

    // en-passant position
    position.from_fen("8/8/8/3Pp3/8/8/8/k6K w - e6 0 1");
    move = position.move_from_uci("d5e6"); // en-passant capture
    ASSERT_TRUE(is_legal_move(move));
    position.make_move(move);
    z = position.get_zobrist_hash();
    EXPECT_EQ(z, rebuild_hash_from_fen(position)) << "En-passant zobrist value mismatch after make";

    // promotion position
    position.from_fen("8/P7/8/8/8/8/8/k6K w - - 0 1");
    move = position.move_from_uci("a7a8q"); // promote to queen
    ASSERT_TRUE(is_legal_move(move));
    position.make_move(move);
    z = position.get_zobrist_hash();
    EXPECT_EQ(z, rebuild_hash_from_fen(position)) << "Promotion zobrist value mismatch after make";
}

TEST(ZobristTests, CollisionCheck) {
    // Basic collision test. For a 500*200 = 1e5 positions, the probability of a collision
    // with 64-bit zobrist keys is around 1 in 10 billion, so any collision found is almost
    // certainly a bug in the zobrist implementation.
    const int games = 500;          // number of random games
    const int moves_per_game = 200; // max moves per game

    Position position;
    std::mt19937_64 rng(42);
    std::unordered_map<uint64_t, FEN> seen;

    // Helper to strip halfmove/fullmove counters from FEN
    auto strip_counters = [](const FEN& fen) -> FEN {
        int spaces = 0;
        for (size_t i = 0; i < fen.size(); ++i) {
            if (fen[i] == ' ') {
                if (++spaces == 4) {
                    return fen.substr(0, i);
                }
            }
        }
        return fen;
    };

    MoveList move_list;
    for (int g = 0; g < games; ++g) {
        position.from_fen(COMPLEX_POSITION);
        for (int m = 0; m < moves_per_game; ++m) {
            move_list.generate<GenerateType::Legal>(position);
            if (move_list.count() == 0) break;

            std::uniform_int_distribution<size_t> dist(0, move_list.count() - 1);
            position.make_move(move_list[dist(rng)]);
            uint64_t key = position.get_zobrist_hash();
            FEN fen = strip_counters(position.to_fen());

            auto it = seen.find(key);
            if (it == seen.end()) {
                seen.emplace(key, fen);
            }
            else if (it->second != fen) {
                // Different FENs sharing same zobrist -> real collision
                FAIL() << "Zobrist collision detected between FENs: '" << it->second << "' and '" << fen << "'\n";
            }
        }
    }
}

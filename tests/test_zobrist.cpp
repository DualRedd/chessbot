#include <unordered_map>
#include <random>

#include "gtest/gtest.h"
#include "core/position.hpp"
#include "core/move_generation.hpp"
#include "positions.hpp"

static std::mt19937 rng;

TEST(ZobristTests, IncrementalMakeUndoConsistency) {
    rng.seed(42);

    Position position, rebuilt;

    // play random moves and store hashes after each make
    MoveList move_list;
    for (const FEN& fen : TEST_POSITIONS) {
        position.from_fen(fen);
        uint64_t orig_hash = position.get_zobrist_hash();

        move_list.generate<GenerateType::Legal>(position);
        ASSERT_FALSE(move_list.count() == 0) << "No legal moves in position: " << fen;
        
        for (Move move : move_list) {
            position.make_move(move);

            // check that rebuilding from FEN reproduces the same zobrist
            uint64_t z1 = position.get_zobrist_hash();
            rebuilt.from_fen(position.to_fen());
            uint64_t z2 = rebuilt.get_zobrist_hash();
            ASSERT_EQ(z1, z2) << "Zobrist mismatch after move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);

            // Check eval matches after undo
            position.undo_move();
            uint64_t z3 = position.get_zobrist_hash();
            ASSERT_EQ(orig_hash, z3) << "Zobrist mismatch after undo move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);
        }
    } 
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

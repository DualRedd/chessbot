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

    // Test that making and undoing moves maintains consistent Zobrist hashes in a variety of positions
    MoveList move_list;
    for (const FEN& fen : TEST_POSITIONS) {
        position.from_fen(fen);
        uint64_t orig_key = position.get_key();
        uint64_t orig_pawn_key = position.get_pawn_key();

        move_list.generate<GenerateType::Legal>(position);
        ASSERT_FALSE(move_list.count() == 0) << "No legal moves in position: " << fen;
        
        for (Move move : move_list) {
            position.make_move(move);

            // check that rebuilding from FEN reproduces the same hashes
            uint64_t k1 = position.get_key();
            uint64_t pk1 = position.get_pawn_key();
            rebuilt.from_fen(position.to_fen());
            uint64_t k2 = rebuilt.get_key();
            uint64_t pk2 = rebuilt.get_pawn_key();
            ASSERT_EQ(k1, k2) << "Full hash mismatch after move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);
            ASSERT_EQ(pk1, pk2) << "Pawn hash mismatch after move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);

            // Check eval matches after undo
            position.undo_move();
            uint64_t k3 = position.get_key();
            uint64_t pk3 = position.get_pawn_key();
            ASSERT_EQ(orig_key, k3) << "Full hash mismatch after undo move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);
            ASSERT_EQ(orig_pawn_key, pk3) << "Pawn hash mismatch after undo move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);
        }
    } 
}

TEST(ZobristTests, CollisionCheck) {
    rng.seed(42);

    // Basic collision test. For a 500*200 = 1e5 positions, the probability of a collision
    // with 64-bit zobrist keys is around 1 in 10 billion, so any collision found is almost
    // certainly a bug in the zobrist implementation.
    const int games = 500;          // number of random games
    const int moves_per_game = 200; // max moves per game

    Position position;
    std::unordered_map<uint64_t, FEN> seen_keys;
    std::unordered_map<uint64_t, FEN> seen_pawn_keys;

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

    auto only_pawn_info = [&position]() -> std::string {
        std::string result;
        for (Square sq = Square::A1; sq <= Square::H8; sq = Square(sq + 1)) {
            Piece piece = position.get_piece_at(sq);
            if (to_type(piece) == PieceType::Pawn) {
                std::string color_str = (piece == Piece::WPawn) ? "W" : "B";
                result += "{" + color_str + "," + std::to_string(file_of(sq)) + "," + std::to_string(rank_of(sq)) + "}";
            }
        }
        return result;
    };

    MoveList move_list;
    for (int g = 0; g < games; ++g) {
        position.from_fen(COMPLEX_POSITION);
        for (int m = 0; m < moves_per_game; ++m) {
            move_list.generate<GenerateType::Legal>(position);
            if (move_list.count() == 0) break;

            position.make_move(move_list[rng() % move_list.count()]);

            uint64_t key = position.get_key();
            FEN fen = strip_counters(position.to_fen());
            auto it = seen_keys.find(key);

            if (it == seen_keys.end()) {
                seen_keys.emplace(key, fen);
            }
            else if (it->second != fen) {
                // Different FENs sharing same zobrist -> real collision
                FAIL() << "Zobrist collision detected between FENs: '" << it->second << "' and '" << fen << "'\n";
            }

            std::string pawn_info = only_pawn_info();
            uint64_t pawn_key = position.get_pawn_key();
            it = seen_pawn_keys.find(pawn_key);

            if (it == seen_pawn_keys.end()) {
                seen_pawn_keys.emplace(pawn_key, pawn_info);
            }
            else if (it->second != pawn_info) {
                // Different FENs sharing same pawn zobrist -> real collision
                FAIL() << "Pawn Zobrist collision detected pawn structures: '" << it->second << "' and '" << pawn_info << "'\n";
            }
        }
    }
}

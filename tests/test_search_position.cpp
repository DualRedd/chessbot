#include <random>

#include "gtest/gtest.h"
#include "engine/search_position.hpp"
#include "core/move_generation.hpp"
#include "positions.hpp"

static std::mt19937 rng;

TEST(SearchPositionTests, InitialEval) {
    SearchPosition ss;
    ss.set_board(CHESS_START_POSITION);
    EXPECT_EQ(ss.get_eval(), 0) << "Initial position should have eval 0.";
}

TEST(SearchPositionTests, MakeUndoConsistency) {
    rng.seed(42);

    SearchPosition ss, rebuilt;
    MoveList move_list;

    for (const FEN& fen : TEST_POSITIONS) {
        ss.set_board(fen);
        int32_t orig_eval = ss.get_eval();

        move_list.generate<GenerateType::Legal>(ss.get_position());
        ASSERT_FALSE(move_list.count() == 0) << "No legal moves in position: " << fen;
        
        for (Move move : move_list) {
            ss.make_move(move);

            // check that rebuilding from FEN reproduces the same eval
            int32_t eval1 = ss.get_eval();
            rebuilt.set_board(ss.get_position().to_fen()); // drop move history
            int32_t eval2 = rebuilt.get_eval();
            ASSERT_EQ(eval1, eval2) << "Eval mismatch after move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);

            // Check eval matches after undo
            ss.undo_move();
            int32_t eval3 = ss.get_eval();
            ASSERT_EQ(orig_eval, eval3) << "Eval mismatch after undo move in position: "
                << fen << ", move: " << MoveEncoding::to_uci(move);
        }
    }
}

// Flip/rotate the FEN 180 degrees and swap piece colors and side to move.
// This produces the position seen from the opposite side such that a
// symmetric eval should be equal for the side to move.
static std::string flip_fen_colors(const std::string& fen) {
    // split fen fields
    std::istringstream iss(fen);
    std::string board_fld, side_fld, castle_fld, ep_fld, half_fld, full_fld;
    iss >> board_fld >> side_fld >> castle_fld >> ep_fld >> half_fld >> full_fld;

    // expand board to 8x8
    char board[8][8];
    int rank = 0;
    std::istringstream br(board_fld);
    std::string rankstr;
    while (std::getline(br, rankstr, '/')) {
        int file = 0;
        for (char c : rankstr) {
            if (std::isdigit(static_cast<unsigned char>(c))) {
                int empt = c - '0';
                for (int i = 0; i < empt; ++i) board[rank][file++] = '.';
            } else {
                board[rank][file++] = c;
            }
        }
        ++rank;
    }

    // rotate 180 deg and swap case for pieces
    char newb[8][8];
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            char c = board[7 - r][7 - f];
            if (c == '.') newb[r][f] = '.';
            else {
                if (std::islower(static_cast<unsigned char>(c)))
                    newb[r][f] = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                else
                    newb[r][f] = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
    }

    // compress back to FEN board string
    std::ostringstream ob;
    for (int r = 0; r < 8; ++r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            char c = newb[r][f];
            if (c == '.') {
                ++empty;
            } else {
                if (empty) { ob << empty; empty = 0; }
                ob << c;
            }
        }
        if (empty) ob << empty;
        if (r != 7) ob << '/';
    }
    std::string new_board = ob.str();

    // flip side to move
    std::string new_side = (side_fld == "w") ? "b" : "w";

    // swap case of castling field (KQkq -> kqKQ style). keep '-' as is.
    std::string new_castle;
    if (castle_fld == "-") {
        new_castle = "-";
    } else {
        new_castle.reserve(castle_fld.size());
        for (char c : castle_fld) {
            if (std::islower(static_cast<unsigned char>(c)))
                new_castle.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            else
                new_castle.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }

    // en-passant: if set, rotate rank: new_rank = 9 - old_rank (e.g. e3 -> e6)
    std::string new_ep = "-";
    if (ep_fld != "-") {
        char file = ep_fld[0];
        char rankc = ep_fld[1];
        char newrankc = static_cast<char>('0' + (9 - (rankc - '0')));
        new_ep = std::string{file, newrankc};
    }

    // halfmove/fullmove keep as-is
    return new_board + " " + new_side + " " + new_castle + " " + new_ep + " " + half_fld + " " + full_fld;
}

TEST(SearchPositionTests, ColorFlippedEvalMatches) {
    SearchPosition ss, flipped;
    for (const FEN& fen : TEST_POSITIONS) {
        // Skip positions that allow castling or en-passant
        std::istringstream iss(fen);
        std::string board_fld, side_fld, castle_fld, ep_fld, half_fld, full_fld;
        iss >> board_fld >> side_fld >> castle_fld >> ep_fld >> half_fld >> full_fld;
        if (castle_fld != "-" || ep_fld != "-")
            continue;

        ss.set_board(fen);
        int32_t eval_orig = ss.get_eval();

        std::string flipped_fen = flip_fen_colors(fen);
        flipped.set_board(flipped_fen);
        int32_t eval_flipped = flipped.get_eval();

        // evaluation from the opposite side should be the negation
        EXPECT_EQ(eval_orig, eval_flipped)
            << "Eval symmetry failed for position:\n  orig: " << fen
            << "\n  flipped: " << flipped_fen
            << "\n  eval_orig=" << eval_orig << " eval_flipped=" << eval_flipped;
    }
}

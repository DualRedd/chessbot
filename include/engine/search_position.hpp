#pragma once

#include "../core/position.hpp"
#include "engine/value_tables.hpp"
#include "engine/pawn_hash_table.hpp"

struct Eval {
    int32_t mg_eval;  // middle game eval
    int32_t eg_eval;  // end game eval
    int32_t phase;    // material on board
};

/**
 * Incremental evaluation wrapper for Position.
 */
class SearchPosition {
public:
    /**
     * Create without setting board state. Call set_board() after.
     */
    SearchPosition();

    /**
     * Set board configuration from a FEN description.
     * @param fen FEN string
     * @throw std::invalid_argument if the FEN string is not valid
     */
    void set_board(const FEN& fen);

    /**
     * @return Position evaluation from the perspective of side to move.
     */
    int32_t get_eval() const;

    /**
     * @return How many times the current position has occurred in the move history.
     * @note e.g. 1 if the current position is the first occurence in the history.
     */
    int repetition_count() const;

    /**
     * @return Number of plies since the last irreversible move (pawn move or capture).
     */
    int plies_since_irreversible_move() const;

    /**
     * @return Current material phase value. Weighted sum of all pieces on the board.
     */
    int32_t material_phase() const;

    /**
     * Make a move on the board.
     * @param move the move.
     * @note Illegal moves can cause internal state to become invalid.
     * There are no safety checks for performance reasons.
     */
    void make_move(Move move);

    /**
     * Undo the last move on this board.
     * @return True if there was a previous move (and it was undone), else false.
     */
    bool undo_move();

    /**
     * Apply a null move on this board (passing the turn).
     */
    void make_null_move();

    /**
     * Undo a null move on this board. Must only be used to undo a previously made null move.
     */
    void undo_null_move();

    /**
     * @return The underlying board.
     */
    const Position& get_position() const;

private:
    /**
     * Piece-Square Table value.
     * @param type piece type
     * @param color player color
     * @param square square index 0-63 (8 * rank + file)
     * @param stage game stage (middlegame/endgame)
     * @return Evaluation value for the piece on the square.
     */
    int32_t _pst_value(PieceType type, Color color, Square square, GamePhase stage) const;

    /**
     * @param type piece type
     * @return Evaluation material value for the piece.
     */
    int32_t _material_value(PieceType type) const; 

    /**
     * Calculate the full evaluation from the current position.
     * @return The evaluation from white's perspective.
     */
    Eval _compute_full_eval();

    /**
     * Evaluate pawn structure.
     * @return Pawn structure evaluation from white's perspective.
     */
    int32_t _eval_pawns() const;

private:
    Position m_position;

    // Evaluation from white's perspective
    std::vector<Eval> m_evals;
    mutable PawnHashTable m_pawn_hash_table;

    // Ply history
    std::vector<uint64_t> m_zobrist_history;
    std::vector<size_t> m_irreversible_move_plies;
};

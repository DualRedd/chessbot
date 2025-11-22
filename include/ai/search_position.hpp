#pragma once

#include "../core/board.hpp"

/**
 * Incremental evaluation wrapper for Board.
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
    double get_eval() const;

    /**
     * @param ordered whether to apply move ordering
     * @return All pseudo-legal moves in the current position.
     */
    std::vector<Move> get_ordered_pseudo_legal_moves() const;

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
     * @return The underlying board.
     */
    const Board& get_board() const;

private:
    /**
     * @param type piece type
     * @return Evaluation material value for the piece.
     */
    double _material_value(PieceType type) const;

    /**
     * Piece-Square Table value.
     * @param type piece type
     * @param color player color
     * @param square square index 0-63 (8 * rank + file)
     * @return Evaluation value for the piece on the square.
     */
    double _pst_value(PieceType type, PlayerColor color, int square) const;

    /**
     * Calculate the full evaluation from the current position.
     * @return The evaluation of the board (from white's perspective).
     */
    double _compute_full_eval();

private:
    Board m_board;

    double m_eval; // Evaluation from white's perspective
    std::vector<double> m_eval_history;
};

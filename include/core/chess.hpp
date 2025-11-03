#pragma once

#include "board.hpp"

class Chess {
public:
    /**
     * Initializes the game with the standard starting position.
     */
    Chess();
    ~Chess() = default;

    /**
     * @param fen FEN notation string describing the board state. See: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
     * @note The default FEN string is the standard starting position.
     */
    void new_board(const std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    /**
     * @return True if the move was executed (it was legal).
     */
    bool play_move(const Move& move);

    /**
     * @return The last move played. No value if no moves have been played on this board.
     */
    std::optional<Move> get_last_move() const;

    /**
     * @return A reference to the board this game is played on.
     */
    const Board& get_board() const;

private:
    Board m_board;
    std::optional<Move> m_last_move;
};

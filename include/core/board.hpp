#pragma once

#include "piece.hpp"

#include <array>
#include <string>
#include <vector>
#include <optional>

constexpr int CHESSBOARD_SIZE = 8;

/**
 * Represents a tile within a board.
 */
struct BoardTile {
    int file;
    int rank;

    BoardTile();
    BoardTile(int file, int rank);
    bool operator==(const BoardTile& other) const;
    bool operator!=(const BoardTile& other) const;

    /**
     * @return True if this tile is within a chessboard's bounds, else False.
     */
    bool valid() const;
};


/**
 * Represents a move within a board.
 */
struct Move {
    BoardTile from;
    BoardTile to;
    PieceType promotion = PieceType::None;

    Move();
    Move(BoardTile from, BoardTile to, PieceType promotion = PieceType::None);

    /**
     * @param uci UCI string describing the move
     */
    Move(const std::string& uci);

    /**
     * Check for equal start and end square and promotion.
     */
    bool operator==(const Move& other) const;

    /**
     * @return True this move is within a chessboard's bounds and the promotion piece is valid, else False.
     */
    bool valid() const;

    /**
     * @return UCI representation of the move
     */
    std::string toUCI() const;
};


class Board {
public:
    Board();

    /**
     * Initialize the board state from a FEN string.
     * @param fen The FEN string describing the position.
     * @throws std::invalid_argument if the FEN string is malformed.
     */
    void setup(const std::string& fen);

    /**
     * @return The board state in FEN notation.
     */
    std::string to_fen() const;

    /**
     * @return The piece at the position
     */
    const Piece& get_piece(BoardTile tile) const;

    /**
     * Apply a move regardless of its legality.
     * 
     * @return True if the move was succesful. False if the piece did not exist.
     */
    bool move_piece(Move move);

    /**
     * @return A list of legal moves for the piece at the given position (none if there is no piece)
     */
    std::vector<Move> get_legal_moves(BoardTile from_tile) const;

    /**
     * @return True if this a legal move in the current board state.
     */
    bool is_legal_move(const Move& move);

    /**
     * @return The current side to move next.
     */
    PlayerColor get_side_to_move() const;

    /**
     * @return A human readable representation of the current board state
     */
    std::string to_string() const;

private:
    std::array<std::array<Piece, CHESSBOARD_SIZE>, CHESSBOARD_SIZE> m_board;
    PlayerColor m_side_to_move;
    bool m_white_king_side_castle_available;
    bool m_black_king_side_castle_available;
    bool m_white_queen_side_castle_available;
    bool m_black_queen_side_castle_available;
    bool m_en_passant_available;
    int m_en_passant_target_file, m_en_passant_target_rank;
    int m_halfmoves, m_fullmoves;
};
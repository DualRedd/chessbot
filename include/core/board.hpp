#pragma once

#include <array>
#include <string>
#include <vector>
#include <optional>

constexpr int BOARD_SIZE = 8;

enum class PieceType {
    None = 0,
    Pawn = 1, 
    Knight = 2, 
    Bishop = 3, 
    Rook = 4, 
    Queen = 5, 
    King = 6
};

enum class PlayerColor {
    None = 0,
    White = 1,
    Black = 2
};

struct Piece {
    PieceType type = PieceType::None;
    PlayerColor color = PlayerColor::None;
    bool operator==(const Piece& other) const;
    bool operator!=(const Piece& other) const;
    struct Hash {
        std::size_t operator()(const Piece& p) const;
    };
};

/**
 * Represents a move within a board.
 */
struct Move {
    int from_file;
    int from_rank;
    int to_file;
    int to_rank;
    std::optional<PieceType> promotion = std::nullopt;

    Move(int from_file, int from_rank, int to_file, int to_rank, std::optional<PieceType> promo = std::nullopt);
    Move(int from_file, int from_rank, int to_file, int to_rank, PieceType promotion);

    /**
     * Construct from UCI string.
     */
    Move(const std::string& uci);

    /**
     * Equal move and possible promotion.
     */
    bool operator==(const Move& other) const;

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
    const Piece& get_piece(int file, int rank) const;

    /**
     * Apply a move regardless of its legality.
     * 
     * @return True if the move was succesful. False if the piece did not exist.
     */
    bool move_piece(Move move);

    /**
     * @return A list of legal moves for the piece at the given position (none if there is no piece)
     */
    std::vector<Move> get_legal_moves(int file, int rank) const;

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
    std::array<std::array<Piece, BOARD_SIZE>, BOARD_SIZE> m_board;
    PlayerColor m_side_to_move;
    bool m_white_king_side_castle_available;
    bool m_black_king_side_castle_available;
    bool m_white_queen_side_castle_available;
    bool m_black_queen_side_castle_available;
    bool m_en_passant_available;
    int m_en_passant_target_file, m_en_passant_target_rank;
    int m_halfmoves, m_fullmoves;
};
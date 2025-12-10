#pragma once

#include <unordered_map>
#include "../core/position.hpp"

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

/**
 * Interface to play chess.
 */
class Chess {
public:
    /** Tile on the chessboard. */
    struct Tile {
        int file;
        int rank;

        Tile();
        Tile(int file, int rank);
        bool operator==(const Tile& other) const;
        bool operator!=(const Tile& other) const;

        /**
         * @return Index 0...63 (rank * 8 + file).
         */
        int to_index() const;

        /**
         * @return True if this is within a chessboard's bounds, else false.
         */
        bool valid() const;
    };

    /** Game state enumeration. */
    enum class GameState {
        NoCheck,                    // No check on the current player
        Check,                      // Current player's king is in check
        Checkmate,                  // Current player's king is in checkmate
        Stalemate,                  // Current player has no legal moves but is not in check
        DrawByFiftyMoveRule,        // Draw by fifty-move rule
        DrawByInsufficientMaterial, // Draw by insufficient material
        DrawByThreefoldRepetition   // Draw by threefold repetition
    };

    /**
     * Helper for creating UCI strings.
     * @param from origin tile
     * @param to target tile
     * @param promotion promotion type (default PieceType::None)
     * @return UCI string describing the move.
     * @throw invalid_argument if the arguments do not describe a valid UCI move.
     */
    static UCI uci_create(const Tile& from, const Tile& to, PieceType promotion = PieceType::None);

    /**
     * Helper for parsing UCI strings.
     * @param uci the UCI string
     * @return Parsed origin tile, target tile and promotion piece.
     * @throw invalid_argument if the UCI is not valid.
     */
    static std::tuple<Tile, Tile, PieceType> uci_parse(const UCI& uci);

public:
    /**
     * Initializes the game with the standard starting position.
     */
    Chess();
    ~Chess();

    /**
     * @param fen FEN notation string describing the board state. See: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
     * @note The default FEN string is the standard starting position.
     */
    void new_board(const FEN& position = CHESS_START_POSITION);

    /**
     * @return FEN notation string describing the current board state. See: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
     */
    FEN get_board_as_fen() const;

    /**
     * @return Which sides turn to move it is currently.
     */
    Color get_side_to_move() const;

    /**
     * @param tile the tile
     * @return The piece at that square. If there is no piece the type field will be PieceType::None.
     */
    Piece get_piece_at(const Tile& tile) const;

    /**
     * @return All legal moves the current board state in UCI format. See: https://en.wikipedia.org/wiki/Universal_Chess_Interface
     */
    std::vector<UCI> get_legal_moves() const;

    /**
     * @param move UCI string describing the move, See: https://en.wikipedia.org/wiki/Universal_Chess_Interface
     * @return True if the move is legal on the current board.
     */
    bool is_legal_move(const UCI& move) const;

    /**
     * @param uci UCI string describing the move, See: https://en.wikipedia.org/wiki/Universal_Chess_Interface
     * @return True if the move was legal (and therefore played), else false.
     */
    bool play_move(const UCI& move);

    /**
     * @return True if there was a previous move and it was undone, else false.
     */
    bool undo_move();

    /**
     * @return The last move played if there is one.
     */
    std::optional<UCI> get_last_move() const;

    /**
     * Get current game status: check, checkmate, stalemate,
     * draw by fifty-move rule, draw by insufficient material, draw by threefold repetition.
     * @return The state of the game as a enum.
     */
    GameState get_game_state() const;

private:
    /**
     * Refetch cached UCI move list.
     */
    void _update_legal_moves();

    /**
     * @return True if there is insufficient material on the board for checkmate, else false.
     * @note This is not comprehensive, only basic cases are covered.
     */
    bool _is_insufficient_material() const;

    /**
     * @return True if the current position has occurred three times, else false.
     */
    bool _is_threefold_repetition() const;

private:
    Position m_position;
    std::vector<UCI> m_legal_moves;

    // Zobrist history for threefold-repetition detection
    std::vector<uint64_t> m_zobrist_history;
    std::unordered_map<uint64_t, int> m_zobrist_counts;
    // FEN history for verification on suspected collisions (rare zobrist collisions)
    std::vector<FEN> m_fen_history;
};

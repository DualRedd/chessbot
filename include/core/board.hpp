#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

#include "types.hpp"

/**
 * Bitboard based chess position class.
 */
class Board {
public:
    // Bitboard type (64 bits)
    using Bitboard = uint64_t;

    Board() = default;
    Board(const Board&) = delete;
    Board& operator=(const Board&) = delete;

    /**
     * Create a board from a FEN description.
     * @param fen FEN string
     * @param allow_illegal_position if true, an illegal position according to normal chess rules can be set without a thrown exception
     * @throw std::invalid_argument if the FEN string is not valid OR the position is illegal and allow_illegal_positions is false. 
     */
    explicit Board(const FEN& fen, bool allow_illegal_position = false);

    /**
     * Copy constructor. Allows to drop move history.
     * @param board other board
     * @param copy_history move history is copied only if this is true
     */
    Board(const Board& other, bool copy_history = true);

    /**
     * Set board configuration from a FEN description. Resets all board state.
     * @param fen FEN string
     * @param allow_illegal_position if true, an illegal position according to normal chess rules can be set without a thrown exception
     * @throw std::invalid_argument if the FEN string is not valid OR the position is illegal and allow_illegal_positions is false. 
     */
    void set_from_fen(const FEN& fen, bool allow_illegal_position = false);

    /**
     * @return Current board state as a FEN string.
     */
    FEN to_fen() const;

    /**
     * @return Current Zobrist hash of the board state.
     */
    uint64_t get_zobrist_hash() const;

    /**
     * @return Which sides turn to move it is currently.
     */
    PlayerColor get_side_to_move() const;

    /**
     * @return The last move played if there is one.
     */
    std::optional<Move> get_last_move() const;

    /**
     * @return The number of halfmoves since the last capture or pawn move.
     */
    uint32_t get_halfmove_clock() const;

    /**
     * @param square index (8 * rank + file)
     * @return The piece at that square. If there is no piece the type field will be PieceType::None.
     */
    Piece get_piece_at(const int square) const;

    /**
     * @return All pseudo-legal moves in the current board state.
     */
    std::vector<Move> generate_pseudo_legal_moves() const;

    /**
     * @return All legal moves in the current board state.
     */
    std::vector<Move> generate_legal_moves() const;

    /**
     * @param square square index (8 * rank + file)
     * @param by player color
     * @return True if the the square is attacked by any of the player's pieces, else false.
     */
    bool is_square_attacked(const int square, const PlayerColor by) const;

    /**
     * @param side White or Black
     * @return True if this side's king(s) is in check, else false.
     */
    bool in_check(const PlayerColor side) const;

    /**
     * Apply a Move on this board.
     * @param move the move.
     * @note Illegal moves can cause internal state to become invalid.
     * There are no safety checks for performance reasons.
     */
    void make_move(const Move move);

    /**
     * Undo the last move on this board.
     * @return True if there was a previous move (and it was undone), else false.
     */
    bool undo_move();

    /**
     * Convert UCI string to a Move on the current board.
     * @throw invalid_argument if the UCI string is invalid.
     * @return The move corresponding to the UCI string.
     * @note The constructed move might not be legal!
     */
    Move move_from_uci(const UCI& uci) const;

private:
    /**
     * Reversable state transition.
     */
    struct StoredState {
        Move move;
        uint8_t castling_rights;
        int8_t en_passant_square;
        uint64_t zobrist;
        uint32_t halfmoves;
        StoredState(Move move, uint8_t castling_rights,
            int8_t en_passant_square, uint64_t zobrist, uint32_t halfmoves);
    };

    /**
     * @param piece piece type
     * @param color player color
     * @param square square index (8 * rank + file)
     * @return Bitboard representing all squares the given piece could attack from the square.
     * Takes into account blocking pieces, but does not consider piece color (only used to determine pawn attack direction).
     */
    Bitboard _attacks_from(const PieceType piece, const PlayerColor color, const int square) const;

    /**
     * Precalculate various attack and move masks.
     */
    static void _precalc();

private:
    std::vector<StoredState> m_state_history;

    Bitboard m_pieces[2][6];         // [color][piece]
    Bitboard m_occupied[2];          // [color]
    Bitboard m_occupied_all;         // all pieces
    PieceType m_piece_on_square[64]; // [square]

    PlayerColor m_side_to_move;
    uint8_t m_castling_rights;       // bitmask: WK=1, WQ=2, BK=4, BQ=8
    int8_t m_en_passant_square;      // 0–63 or -1 if none
    uint32_t m_halfmoves;
    uint32_t m_fullmoves;
    uint64_t m_zobrist;

private:
    // Precalculated masks and values
    static inline Bitboard MASK_SQUARE[64];          // [square]
    static inline Bitboard MASK_PAWN_ATTACKS[2][64]; // [color][square]
    static inline Bitboard MASK_KNIGHT_ATTACKS[64];  // [square]
    static inline Bitboard MASK_KING_ATTACKS[64];    // [square]
    static inline Bitboard MASK_BISHOP[64];          // [square]
    static inline Bitboard MASK_ROOK[64];            // [square]

    static inline Bitboard MASK_CASTLE_CLEAR[2][2];  // [color][0=queenside,1=kingside]
    static inline uint8_t CASTLE_FLAG[2][2];         // [color][0=queenside,1=kingside]
    static inline uint8_t ROOK_SQUARE[2][2];         // [color][0=queenside,1=kingside]
    static inline uint8_t KING_SQUARE[2];            // [color]

    static inline uint64_t ZOBRIST_PIECE[2][6][64]; // [color][piece][square]
    static inline uint64_t ZOBRIST_CASTLING[16];    // [castling rights bitmask]
    static inline uint64_t ZOBRIST_EP[8];           // [file of en passant square]
    static inline uint64_t ZOBRIST_SIDE;            // side to move

    struct Initializer {
        Initializer() {
            _precalc();
        }
    };
    static inline Initializer _init_once;
};

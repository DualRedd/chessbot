#pragma once

#include <vector>
#include <array>
#include <string>
#include <optional>

#include "bitboard.hpp"

/**
 * Bitboard based chess position class.
 */
class Position {
public:
    // Bitboard type (64 bits)
    using Bitboard = uint64_t;

    Position() = default;
    Position(const Position&) = delete;
    Position& operator=(const Position&) = delete;

    /**
     * Create a board from a FEN description.
     * @param fen FEN string
     * @param allow_illegal_position if true, an illegal position according to normal chess rules can be set without a thrown exception
     * @throw std::invalid_argument if the FEN string is not valid OR the position is illegal and allow_illegal_positions is false.
     */
    explicit Position(const FEN& fen);

    /**
     * Copy constructor. Allows to drop move history.
     * @param board other board
     * @param copy_history move history is copied only if this is true
     */
    Position(const Position& other, bool copy_history = true);

    /**
     * Set board configuration from a FEN description. Resets all board state.
     * @param fen FEN string
     * @throw std::invalid_argument if the FEN string is not valid OR the position is illegal and allow_illegal_positions is false.
     */
    void from_fen(const FEN& fen);

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
    Color get_side_to_move() const;

    /**
     * @return The last move played if there is one.
     */
    std::optional<Move> get_last_move() const;

    /**
     * @return The number of halfmoves since the last capture or pawn move.
     */
    uint32_t get_halfmove_clock() const;

    /**
     * @return The number of fullmoves since the start of the game.
     */
    uint32_t get_fullmove_clock() const;

    /**
     * @param square index (8 * rank + file)
     * @return The piece at that square. If there is no piece the type field will be PieceType::None.
     */
    Piece get_piece_at(Square square) const;

    Bitboard get_pieces(Color color, PieceType type) const;
    Bitboard get_pieces(Color color) const;
    Bitboard get_pieces() const;
    Square get_en_passant_square() const;

    /**
     * @param side White or Black
     * @return True if this side's king(s) is in check, else false.
     */
    bool in_check(Color side) const;

    Bitboard attackers(Color side, Square square, Bitboard occupied) const;
    bool attackers_exist(Color side, Square square, Bitboard occupied) const;
    Bitboard get_pinned() const;

    bool can_castle(Color side, CastlingSide castle_side) const;

    /**
     * Apply a Move on this board.
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

    PieceType to_capture(Move move) const {
        Square to = MoveEncoding::to_sq(move);
        if (MoveEncoding::move_type(move) == MoveType::EnPassant) {
            Square capture_square = (m_side_to_move == Color::White) ? (to + Shift::Down) : (to + Shift::Up);
            return to_type(get_piece_at(capture_square));
        }
        return to_type(get_piece_at(to));
    }

    PieceType to_moved(Move move) const {
        Square from = MoveEncoding::from_sq(move);
        return to_type(get_piece_at(from));
    }

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
        Square en_passant_square;
        Piece captured_piece;
        uint64_t zobrist;
        Bitboard pinned;
        uint32_t halfmoves;
        StoredState(Move move, Piece captured_piece, uint8_t castling_rights,
            Square en_passant_square, uint32_t halfmoves, uint64_t zobristm, Bitboard pinned);
    };

    void _calculate_pinned(Color side) const;

private:
    std::vector<StoredState> m_state_history;

    Bitboard m_pieces[2][6];         // [color][piece]
    Bitboard m_occupied[2];          // [color]
    Bitboard m_occupied_all;         // all pieces
    Piece m_piece_on_square[64];     // [square]

    mutable Bitboard m_pinned;       // pinned pieces
    mutable bool m_pinned_calculated;

    Color m_side_to_move;
    uint8_t m_castling_rights;       // bitmask: WK=1, WQ=2, BK=4, BQ=8
    Square m_en_passant_square;      // 0–63 or -1 if none
    uint32_t m_halfmoves;
    uint32_t m_fullmoves;
    uint64_t m_zobrist;
};


inline Bitboard Position::get_pieces(Color color, PieceType type) const {
    return m_pieces[+color][+type];
}

inline Bitboard Position::get_pieces(Color color) const {
    return m_occupied[+color];
}

inline Bitboard Position::get_pieces() const {
    return m_occupied_all;
}

inline Square Position::get_en_passant_square() const {
    return m_en_passant_square;
}

inline Bitboard Position::get_pinned() const {
    if (!m_pinned_calculated) {
        _calculate_pinned(m_side_to_move);
        m_pinned_calculated = true;
    }
    return m_pinned;
}

inline uint64_t Position::get_zobrist_hash() const {
    return m_zobrist;
}

inline Color Position::get_side_to_move() const {
    return m_side_to_move;
}

inline std::optional<Move> Position::get_last_move() const {
    if(m_state_history.size() == 0) return std::nullopt;
    else return m_state_history.back().move;
}

inline uint32_t Position::get_halfmove_clock() const {
    return m_halfmoves;
}

inline uint32_t Position::get_fullmove_clock() const {
    return m_fullmoves;
}

inline Piece Position::get_piece_at(Square square) const {
    return m_piece_on_square[+square];
}

inline bool Position::can_castle(Color side, CastlingSide castle_side) const {
    if (side == Color::White) {
        return castle_side == CastlingSide::KingSide ?
                (m_castling_rights & +CastlingFlag::WhiteKingSide) != 0
                : (m_castling_rights & +CastlingFlag::WhiteQueenSide) != 0;
    } else {
        return castle_side == CastlingSide::KingSide ?
                (m_castling_rights & +CastlingFlag::BlackKingSide) != 0
                : (m_castling_rights & +CastlingFlag::BlackQueenSide) != 0;
    }
}

inline Bitboard Position::attackers(Color side, Square square, Bitboard occupied) const {
    Color opp = opponent(side);
    Bitboard attackers = 0ULL;

    Bitboard bishops = get_pieces(side, PieceType::Bishop) | get_pieces(side, PieceType::Queen);
    Bitboard rooks = get_pieces(side, PieceType::Rook) | get_pieces(side, PieceType::Queen);

    // Pawn attacks
    attackers |= MASK_PAWN_ATTACKS[+opp][+square] & get_pieces(side, PieceType::Pawn);
    attackers |= MASK_KNIGHT_ATTACKS[+square] & get_pieces(side, PieceType::Knight);
    attackers |= MASK_KING_ATTACKS[+square] & get_pieces(side, PieceType::King);
    attackers |= attacks_from<PieceType::Bishop>(square, occupied) & bishops;
    attackers |= attacks_from<PieceType::Rook>(square, occupied) & rooks;

    return attackers;
}

inline bool Position::attackers_exist(Color side, Square square, Bitboard occupied) const {
    Color opp = opponent(side);

    if (MASK_PAWN_ATTACKS[+opp][+square] & get_pieces(side, PieceType::Pawn))
        return true;
    if (MASK_KNIGHT_ATTACKS[+square] & get_pieces(side, PieceType::Knight))
        return true;
    if (MASK_KING_ATTACKS[+square] & get_pieces(side, PieceType::King))
        return true;

    Bitboard bishops = get_pieces(side, PieceType::Bishop) | get_pieces(side, PieceType::Queen);
    if (attacks_from<PieceType::Bishop>(square, occupied) & bishops)
        return true;

    Bitboard rooks = get_pieces(side, PieceType::Rook) | get_pieces(side, PieceType::Queen);
    if (attacks_from<PieceType::Rook>(square, occupied) & rooks)
        return true;

    return false;
}


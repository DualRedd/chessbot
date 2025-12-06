#pragma once

#include <vector>
#include <array>
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
     * @return The halfmove clock.
     */
    uint32_t get_halfmove_clock() const;

    /**
     * @return The fullmove clock.
     */
    uint32_t get_fullmove_clock() const;

    /**
     * @param square the square to query
     * @return The piece at that square. If there is no piece Piece::None is returned.
     */
    Piece get_piece_at(Square square) const;

    /**
     * @param color the color to query
     * @param type the piece type to query
     * @return Bitboard for the given color and piece type.
     */
    Bitboard get_pieces(Color color, PieceType type) const;

    /**
     * @param color the color to query
     * @return Bitboard for the given color.
     */
    Bitboard get_pieces(Color color) const;

    /**
     * @param type the piece type to query
     * @return Bitboard for the given piece type.
     */
    Bitboard get_pieces(PieceType type) const;

    /**
     * @return Bitboard for all pieces.
     */
    Bitboard get_pieces() const;

    /**
     * @return En passant square if available, else Square::None.
     */
    Square get_en_passant_square() const;

    /**
     * @param side White or Black
     * @return True if this side's king is in check, else false.
     */
    bool in_check(Color side) const;

    /**
     * @return True if current side to move's king is in check, else false.
     */
    bool in_check() const;

    /**
     * @param side the side of the attackers
     * @param square the square to query
     * @param occupied the occupancy bitboard to consider for sliders
     * @return Bitboard of attackers from the given side to the given square.
     */
    Bitboard attackers(Color side, Square square, Bitboard occupied) const;

    /**
     * @param side the side of the attackers
     * @param square the square being attacked
     * @param occupied the occupancy bitboard to consider for sliders
     * @return True if there is at least one attacker from the given side to the given square, else false.
     */
    bool attackers_exist(Color side, Square square, Bitboard occupied) const;

    /**
     * @param side the side to query
     * @return Bitboard of check blockers for the given side (can be opponent pieces too).
     */
    Bitboard get_king_blockers(Color side) const;

    /**
     * @param side the side to query
     * @return Bitboard of pinners for the given side.
     */
    Bitboard get_pinners(Color side) const;

    /**
     * @param side the color to query
     * @param castle_side the castling side to query
     * @return True if castling is still available for the given side and castling side.
     * @note This does not check for legality of the castling move itself!
     */
    bool has_castle(Color side, CastlingSide castle_side) const;

    /**
     * @param move the move to evaluate
     * @return Static Exchange Evaluation (SEE) value for the given move.
     */
    uint32_t static_exchange_evaluation(Move move) const;

    /**
     * Apply a Move on this board.
     * @param move the move.
     * @note Illegal moves can cause internal state to become invalid.
     * There are no safety checks for performance.
     */
    void make_move(Move move);

    /**
     * Undo the last move on this board.
     * @return True if there was a previous move (and it was undone), else false.
     */
    bool undo_move();

    /**
     * @return The piece type being captured by the given move, or Piece::None.
     */
    PieceType to_capture(Move move) const;

    /**
     * @return The piece type being moved by the given move.
     */
    PieceType to_moved(Move move) const;

    /**
     * Convert UCI string to a Move on the current board.
     * @throw invalid_argument if the UCI string is invalid.
     * @return The move corresponding to the UCI string.
     * @note The constructed move might not be legal!
     */
    Move move_from_uci(const UCI& uci) const;

private:
    /**
     * Reversable state transition. 48 bytes, no padding.
     */
    struct StoredState {
        uint64_t zobrist;
        Move move;
        Piece captured_piece;
        uint8_t castling_rights;
        Square en_passant_square;
        uint8_t halfmoves;
        std::array<bool, 2> pins_computed;
        std::array<Bitboard, 2> king_blockers;
        std::array<Bitboard, 2> pinners;
        StoredState(Move move, Piece captured_piece, uint8_t castling_rights,
            Square en_passant_square, uint8_t halfmoves, uint64_t zobrist,
            std::array<Bitboard, 2> king_blockers, std::array<Bitboard, 2> pinners,
            std::array<bool, 2> pins_computed);
    };

    // Helper to compute pins and blockers for the given side
    void _compute_pins(Color side) const;

private:
    std::vector<StoredState> m_state_history;

    Bitboard m_pieces_by_type[7];   // [piece type]
    Bitboard m_pieces_by_color[2];  // [color]
    Piece m_piece_on_square[64];    // [square]
    
    mutable std::array<Bitboard, 2> m_king_blockers; // [color]
    mutable std::array<Bitboard, 2> m_pinners;       // [color]
    mutable std::array<bool, 2> m_pins_computed;     // [color]

    Color m_side_to_move;
    uint8_t m_castling_rights;      // bitmask: WK=1, WQ=2, BK=4, BQ=8
    Square m_en_passant_square;     // 0–63 or -1 if none
    uint8_t m_halfmoves;
    uint32_t m_fullmoves;
    uint64_t m_zobrist;
};


inline Bitboard Position::get_pieces(Color color, PieceType type) const {
    return m_pieces_by_type[+type] & m_pieces_by_color[+color];
}

inline Bitboard Position::get_pieces(Color color) const {
    return m_pieces_by_color[+color];
}

inline Bitboard Position::get_pieces(PieceType type) const {
    return m_pieces_by_type[+type];
}

inline Bitboard Position::get_pieces() const {
    return m_pieces_by_type[+PieceType::All];
}

inline Square Position::get_en_passant_square() const {
    return m_en_passant_square;
}

inline Bitboard Position::get_king_blockers(Color side) const {
    if (!m_pins_computed[+side]) {
        _compute_pins(side);
        m_pins_computed[+side] = true;
    }
    return m_king_blockers[+side];
}

inline Bitboard Position::get_pinners(Color side) const {
    if (!m_pins_computed[+side]) {
        _compute_pins(side);
        m_pins_computed[+side] = true;
    }
    return m_pinners[+side];
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

inline bool Position::has_castle(Color side, CastlingSide castle_side) const {
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
    const Color opp = opponent(side);
    const Bitboard bishops = get_pieces(side, PieceType::Bishop) | get_pieces(side, PieceType::Queen);
    const Bitboard rooks = get_pieces(side, PieceType::Rook) | get_pieces(side, PieceType::Queen);

    Bitboard attackers = 0ULL;
    attackers |= MASK_PAWN_ATTACKS[+opp][+square] & get_pieces(side, PieceType::Pawn);
    attackers |= MASK_KNIGHT_ATTACKS[+square] & get_pieces(side, PieceType::Knight);
    attackers |= MASK_KING_ATTACKS[+square] & get_pieces(side, PieceType::King);
    attackers |= attacks_from<PieceType::Bishop>(square, occupied) & bishops;
    attackers |= attacks_from<PieceType::Rook>(square, occupied) & rooks;

    return attackers;
}

inline bool Position::attackers_exist(Color side, Square square, Bitboard occupied) const {
    const Color opp = opponent(side);
    const Bitboard bishops = get_pieces(side, PieceType::Bishop) | get_pieces(side, PieceType::Queen);
    const Bitboard rooks = get_pieces(side, PieceType::Rook) | get_pieces(side, PieceType::Queen);

    if (MASK_PAWN_ATTACKS[+opp][+square] & get_pieces(side, PieceType::Pawn))
        return true;
    if (MASK_KNIGHT_ATTACKS[+square] & get_pieces(side, PieceType::Knight))
        return true;
    if (MASK_KING_ATTACKS[+square] & get_pieces(side, PieceType::King))
        return true;
    if (attacks_from<PieceType::Bishop>(square, occupied) & bishops)
        return true;
    if (attacks_from<PieceType::Rook>(square, occupied) & rooks)
        return true;

    return false;
}

inline PieceType Position::to_capture(Move move) const {
    const Square to = MoveEncoding::to_sq(move);
    if (MoveEncoding::move_type(move) == MoveType::EnPassant) {
        const Square capture_square = (m_side_to_move == Color::White) ? (to + Shift::Down) : (to + Shift::Up);
        return to_type(get_piece_at(capture_square));
    }
    return to_type(get_piece_at(to));
}

inline PieceType Position::to_moved(Move move) const {
    const Square from = MoveEncoding::from_sq(move);
    return to_type(get_piece_at(from));
}

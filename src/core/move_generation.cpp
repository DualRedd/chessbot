#include "core/move_generation.hpp"

namespace {

template<Shift shift>
inline Move* add_pawn_moves(Bitboard to_bitboard, Move* move_list) {
    while (to_bitboard) {
        Square to_sq = lsb(to_bitboard);
        *move_list++ = MoveEncoding::encode<MoveType::Normal>(to_sq - shift, to_sq);
        pop_lsb(to_bitboard);
    }
    return move_list;
}

template<Shift shift>
inline Move* add_promotion_moves(Bitboard to_bitboard, Move* move_list) {
    while (to_bitboard) {
        Square to_sq = lsb(to_bitboard);
        for (PieceType promo : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, promo);
        }
        pop_lsb(to_bitboard);
    }
    return move_list;
}

inline Move* add_moves(Bitboard to_bitboard, Square from_sq, Move* move_list) {
    while (to_bitboard) {
        Square to_sq = lsb(to_bitboard);
        *move_list++ = MoveEncoding::encode<MoveType::Normal>(from_sq, to_sq);
        pop_lsb(to_bitboard);
    }
    return move_list;
}

template<Color side, PieceType type>
inline Move* generate_piece_moves(const Position& pos, Move* move_list) {
    if constexpr (type != PieceType::Pawn) {
        Bitboard pieces = pos.get_pieces(side, type);
        Bitboard targets = ~pos.get_pieces(side);
        while (pieces) {
            Square from_sq = lsb(pieces);
            Bitboard attacks = attacks_from<type>(from_sq, pos.get_pieces()) & targets;
            move_list = add_moves(attacks, from_sq, move_list);
            pop_lsb(pieces);
        }
    }
    else { // Pawn
        constexpr Color opp = opponent(side);
        constexpr Bitboard rank7 = (side == Color::White) ? RANK_7 : RANK_2;
        constexpr Bitboard rank3 = (side == Color::White) ? RANK_3 : RANK_6;

        Bitboard pawns_on_rank_7 = pos.get_pieces(side, PieceType::Pawn) & rank7;
        Bitboard pawns_not_on_rank_7 = pos.get_pieces(side, PieceType::Pawn) & ~rank7;

        // Forward moves
        constexpr Shift fw = (side == Color::White) ? Shift::Up : Shift::Down;
        constexpr Shift dfw = (side == Color::White) ? Shift::DoubleUp : Shift::DoubleDown;
        Bitboard single_step = shift_bb<fw>(pawns_not_on_rank_7) & ~pos.get_pieces();
        Bitboard double_step = shift_bb<fw>(single_step & rank3) & ~pos.get_pieces();
        move_list = add_pawn_moves<fw>(single_step, move_list);
        move_list = add_pawn_moves<dfw>(double_step, move_list);

        // Captures
        constexpr Shift left = (side == Color::White) ? Shift::UpLeft : Shift::DownLeft;
        constexpr Shift right = (side == Color::White) ? Shift::UpRight : Shift::DownRight;
        Bitboard left_captures = shift_bb<left>(pawns_not_on_rank_7) & pos.get_pieces(opp);
        Bitboard right_captures = shift_bb<right>(pawns_not_on_rank_7) & pos.get_pieces(opp);
        move_list = add_pawn_moves<left>(left_captures, move_list);
        move_list = add_pawn_moves<right>(right_captures, move_list);
       
        // En passant
        if (pos.get_en_passant_square() != Square::None) {
            Bitboard capturers = MASK_PAWN_ATTACKS[+opp][+pos.get_en_passant_square()] & pos.get_pieces(side, PieceType::Pawn);
            while(capturers) {
                Square from_sq = lsb(capturers);
                *move_list++ = MoveEncoding::encode<MoveType::EnPassant>(from_sq, pos.get_en_passant_square());
                pop_lsb(capturers);
            }
        }

        // Promotions
        Bitboard promo_single_step = shift_bb<fw>(pawns_on_rank_7) & ~pos.get_pieces();
        Bitboard promo_left_captures = shift_bb<left>(pawns_on_rank_7) & pos.get_pieces(opp);
        Bitboard promo_right_captures = shift_bb<right>(pawns_on_rank_7) & pos.get_pieces(opp);
        move_list = add_promotion_moves<fw>(promo_single_step, move_list);
        move_list = add_promotion_moves<left>(promo_left_captures, move_list);
        move_list = add_promotion_moves<right>(promo_right_captures, move_list);
    }

    if constexpr (type == PieceType::King) {
        if (!pos.in_check(side))  {
            Square king_sq = king_start_square(side);
            if (pos.can_castle(side, CastlingSide::KingSide)
                && (pos.get_pieces() & (MASK_CASTLE_CLEAR[+side][+CastlingSide::KingSide])) == 0
                && !pos.attackers_exist(opponent(side), king_sq+1, pos.get_pieces())) {
                *move_list++ = MoveEncoding::encode<MoveType::Castle>(king_sq, king_sq+2);
            }
            if (pos.can_castle(side, CastlingSide::QueenSide)
                && (pos.get_pieces() & (MASK_CASTLE_CLEAR[+side][+CastlingSide::QueenSide])) == 0
                && !pos.attackers_exist(opponent(side), king_sq-1, pos.get_pieces())) {
                *move_list++ = MoveEncoding::encode<MoveType::Castle>(king_sq, king_sq-2);
            }
        }
    }

    return move_list;
}

template<Color side>
inline Move* generate_moves_for_side(const Position& pos, Move* move_list) {
    move_list = generate_piece_moves<side, PieceType::Pawn>(pos, move_list);
    move_list = generate_piece_moves<side, PieceType::Knight>(pos, move_list);
    move_list = generate_piece_moves<side, PieceType::Bishop>(pos, move_list);
    move_list = generate_piece_moves<side, PieceType::Rook>(pos, move_list);
    move_list = generate_piece_moves<side, PieceType::Queen>(pos, move_list);
    move_list = generate_piece_moves<side, PieceType::King>(pos, move_list);
    return move_list;
}

template<Color side>
inline bool is_legal_move(const Position& pos, Move move) {
    // https://chess.stackexchange.com/questions/15043/building-a-chess-using-magic-bitboard-how-do-i-know-if-the-movement-is-valid
    constexpr Color opp = opponent(side);
    Square from = MoveEncoding::from_sq(move);
    Square to = MoveEncoding::to_sq(move);
    MoveType move_type = MoveEncoding::move_type(move);

    // King move is legal if the destination square is not attacked
    // Handles castling also (other squares are checked during move generation)
    if (to_type(pos.get_piece_at(from)) == PieceType::King) {
        return !pos.attackers_exist(opp, to, pos.get_pieces() ^ MASK_SQUARE[+from]);
    }

    // Get checker count
    Square king_sq = lsb(pos.get_pieces(side, PieceType::King));
    Bitboard att = pos.attackers(opp, king_sq, pos.get_pieces());
    int attackers_count = popcount(att);
    if (attackers_count >= 2) {
        // only king moves are legal
        return false;
    }

    // en passant special case: simulate occupancy after capture
    if (move_type == MoveType::EnPassant) {
        Square capture_square = (side == Color::White) ? (to + Shift::Down) : (to + Shift::Up);
        // en passant must capture possible checker (the checker is a pawn, discovered check impossible)
        if (attackers_count == 1 && lsb(att) != capture_square) {
            return false;
        }

        Bitboard occ = pos.get_pieces() ^ MASK_SQUARE[+from] ^ MASK_SQUARE[+capture_square] | MASK_SQUARE[+to];
        // only need to check sliding attackers
        if (attacks_from<PieceType::Rook>(king_sq, occ) & (pos.get_pieces(opp, PieceType::Rook) | pos.get_pieces(opp, PieceType::Queen)))
            return false;
        if (attacks_from<PieceType::Bishop>(king_sq, occ) & (pos.get_pieces(opp, PieceType::Bishop) | pos.get_pieces(opp, PieceType::Queen)))
            return false;
        return true;
    }

    bool is_pinned = (pos.get_pinned() & MASK_SQUARE[+from]) != 0ULL;
    bool moves_on_line = (MASK_LINE[+from][+to] & MASK_SQUARE[+king_sq]) != 0ULL;
    if (attackers_count == 1) {
        // legal to capture or block the checker
        Square checker = lsb(att);
        Bitboard allowed_to = MASK_SQUARE[+checker] | MASK_BETWEEN[+checker][+king_sq];
        return (!is_pinned && MASK_SQUARE[+to] & allowed_to) || (is_pinned && moves_on_line && to == checker);
    }

    // Any other move is legal if the piece is not pinned or moves along the pin line
    return (!is_pinned || moves_on_line);
}

} // namespace

Move* generate_pseudo_legal_moves(const Position& pos, Move* move_list) {
    if (pos.get_side_to_move() == Color::White) {
        move_list = generate_moves_for_side<Color::White>(pos, move_list);
    } else {
        move_list = generate_moves_for_side<Color::Black>(pos, move_list);
    }
    return move_list;
}

Move* generate_legal_moves(const Position& pos, Move* move_list) {
    Move* cur = move_list;
    if (pos.get_side_to_move() == Color::White) {
        move_list = generate_moves_for_side<Color::White>(pos, move_list);
        // compress to legal
        for (Move* move_ptr = cur; move_ptr != move_list; ++move_ptr) {
            if (is_legal_move<Color::White>(pos, *move_ptr)) {
                *cur++ = *move_ptr;
            }
        }
    }
    else {
        move_list = generate_moves_for_side<Color::Black>(pos, move_list);
        // compress to legal
        for (Move* move_ptr = cur; move_ptr != move_list; ++move_ptr) {
            if (is_legal_move<Color::Black>(pos, *move_ptr)) {
                *cur++ = *move_ptr;
            }
        }
    }
    return cur;
}


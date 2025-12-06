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

template<GenerateType gen_type, Shift shift, bool is_capture>
inline Move* add_promotion_moves(Bitboard to_bitboard, Move* move_list) {
    while (to_bitboard) {
        Square to_sq = lsb(to_bitboard);
        if constexpr (gen_type == GenerateType::PseudoLegal || gen_type == GenerateType::Evasions) {
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Queen);
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Rook);
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Bishop);
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Knight);
        }
        else if constexpr (gen_type == GenerateType::Captures) {
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Queen);
            if constexpr (is_capture) {
                *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Rook);
                *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Bishop);
                *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Knight);
            }
        }
        else if constexpr (gen_type == GenerateType::Quiets && !is_capture) {
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Rook);
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Bishop);
            *move_list++ = MoveEncoding::encode<MoveType::Promotion>(to_sq - shift, to_sq, PieceType::Knight);
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

template<GenerateType gen_type, Color side>
inline Move* generate_pawn_moves(const Position& pos, Move* move_list, Bitboard targets) {
    constexpr Color opp = opponent(side);
    constexpr Bitboard rank7 = (side == Color::White) ? RANK_7 : RANK_2;
    constexpr Bitboard rank3 = (side == Color::White) ? RANK_3 : RANK_6;
    constexpr Shift forward = (side == Color::White) ? Shift::Up : Shift::Down;
    constexpr Shift double_forward = (side == Color::White) ? Shift::DoubleUp : Shift::DoubleDown;
    constexpr Shift up_left = (side == Color::White) ? Shift::UpLeft : Shift::DownLeft;
    constexpr Shift up_right = (side == Color::White) ? Shift::UpRight : Shift::DownRight;

    const Bitboard empty = ~pos.get_pieces();
    const Bitboard targets_opp = gen_type == GenerateType::Evasions ? pos.get_pieces(opp) & targets : pos.get_pieces(opp);
    const Bitboard pawns_on_rank_7 = pos.get_pieces(side, PieceType::Pawn) & rank7;
    const Bitboard pawns_not_on_rank_7 = pos.get_pieces(side, PieceType::Pawn) & ~rank7;

    if constexpr (gen_type != GenerateType::Captures) {
        // Forward moves
        Bitboard single_step = shift_bb<forward>(pawns_not_on_rank_7) & empty;
        Bitboard double_step = shift_bb<forward>(single_step & rank3) & empty;
        if constexpr (gen_type == GenerateType::Evasions) {
            single_step &= targets;
            double_step &= targets;
        }
        move_list = add_pawn_moves<forward>(single_step, move_list);
        move_list = add_pawn_moves<double_forward>(double_step, move_list);
    }

    // Promotions
    if (pawns_on_rank_7 != 0ULL) {
        // Capture should include queen promotion
        // Quiet should exclude queen promotion
        Bitboard promo_single_step = shift_bb<forward>(pawns_on_rank_7) & empty;
        if constexpr (gen_type == GenerateType::Evasions) {
            promo_single_step &= targets;
        }
        Bitboard promo_left_captures = shift_bb<up_left>(pawns_on_rank_7) & targets_opp;
        Bitboard promo_right_captures = shift_bb<up_right>(pawns_on_rank_7) & targets_opp;
        move_list = add_promotion_moves<gen_type, forward, false>(promo_single_step, move_list);
        move_list = add_promotion_moves<gen_type, up_left, true>(promo_left_captures, move_list);
        move_list = add_promotion_moves<gen_type, up_right, true>(promo_right_captures, move_list);
    }

    if constexpr (gen_type == GenerateType::PseudoLegal || gen_type == GenerateType::Captures || gen_type == GenerateType::Evasions) {
        // Captures
        Bitboard left_captures = shift_bb<up_left>(pawns_not_on_rank_7) & targets_opp;
        Bitboard right_captures = shift_bb<up_right>(pawns_not_on_rank_7) & targets_opp;
        move_list = add_pawn_moves<up_left>(left_captures, move_list);
        move_list = add_pawn_moves<up_right>(right_captures, move_list);
        
        // En passant
        if (pos.get_en_passant_square() != Square::None) {
            // move that makes en passant available cannot also cause a discovered check 
            if (gen_type != GenerateType::Evasions || (targets & MASK_SQUARE[+(pos.get_en_passant_square() + forward)]) == 0ULL) {
                // If this is an evasion generation, now we know the pawn is the checker (if there is one)
                Bitboard capturers = MASK_PAWN_ATTACKS[+opp][+pos.get_en_passant_square()] & pawns_not_on_rank_7;
                if (capturers) {
                    *move_list++ = MoveEncoding::encode<MoveType::EnPassant>(lsb(capturers), pos.get_en_passant_square());
                    pop_lsb(capturers);
                }
                if (capturers) {
                    *move_list++ = MoveEncoding::encode<MoveType::EnPassant>(lsb(capturers), pos.get_en_passant_square());
                }
            } // else: If generating evasions, we know the check was discovered, so en passant cannot be legal here
        }
    }

    return move_list;
}

template<Color side, PieceType type>
inline Move* generate_piece_moves(const Position& pos, Move* move_list, Bitboard targets) {
    static_assert(type != PieceType::Pawn, "Use generate_pawn_moves for pawns");
    static_assert(type != PieceType::King, "Handle king moves separately");

    Bitboard pieces = pos.get_pieces(side, type);
    while (pieces) {
        Square from_sq = lsb(pieces);
        Bitboard attacks = attacks_from<type>(from_sq, pos.get_pieces()) & targets;
        move_list = add_moves(attacks, from_sq, move_list);
        pop_lsb(pieces);
    }

    return move_list;
}

// Generates pseudo-legal moves for the given side and generation type
template<GenerateType gen_type, Color side>
inline Move* generate_moves_for_side(const Position& pos, Move* move_list) {
    static_assert(gen_type != GenerateType::Legal);

    // When in check only evasion generation should be called
    assert(!(pos.in_check() && gen_type != GenerateType::Evasions));

    constexpr Color opp = opponent(side);
    const Square king_sq = lsb(pos.get_pieces(side, PieceType::King));
    Bitboard att = pos.attackers(opp, king_sq, pos.get_pieces());
    Bitboard targets;

    // If gen type is evasions and there are more than 1 attackers, only king moves are needed
    if (gen_type != GenerateType::Evasions || !more_than_1bit(att)) {
        assert(!(gen_type == GenerateType::Evasions && popcount(att) != 1));

        if constexpr (gen_type == GenerateType::Captures) {
            targets = pos.get_pieces(opp);
        }
        else if constexpr (gen_type == GenerateType::Evasions) {
            Square attacker_sq = lsb(att);
            targets = MASK_SQUARE[+attacker_sq] | MASK_BETWEEN[+king_sq][+attacker_sq];
        }
        else if constexpr (gen_type == GenerateType::Quiets) {
            targets = ~pos.get_pieces();
        }
        else { // PseudoLegal
            targets = ~pos.get_pieces(side);
        }

        move_list = generate_pawn_moves<gen_type, side>(pos, move_list, targets);
        move_list = generate_piece_moves<side, PieceType::Knight>(pos, move_list, targets);
        move_list = generate_piece_moves<side, PieceType::Bishop>(pos, move_list, targets);
        move_list = generate_piece_moves<side, PieceType::Rook>(pos, move_list, targets);
        move_list = generate_piece_moves<side, PieceType::Queen>(pos, move_list, targets);
    }

    // King moves
    Bitboard attacks = MASK_KING_ATTACKS[+king_sq] & (gen_type == GenerateType::Evasions ? ~pos.get_pieces(side) : targets);
    move_list = add_moves(attacks, king_sq, move_list);

    // Castling
    if ((gen_type == GenerateType::PseudoLegal || gen_type == GenerateType::Quiets) && att == 0ULL) {
        Square king_sq = king_start_square(side);
        if (pos.has_castle(side, CastlingSide::KingSide)
            && (pos.get_pieces() & (MASK_CASTLE_CLEAR[+side][+CastlingSide::KingSide])) == 0
            && !pos.attackers_exist(opponent(side), king_sq+1, pos.get_pieces())) {
            *move_list++ = MoveEncoding::encode<MoveType::Castle>(king_sq, king_sq+2);
        }
        if (pos.has_castle(side, CastlingSide::QueenSide)
            && (pos.get_pieces() & (MASK_CASTLE_CLEAR[+side][+CastlingSide::QueenSide])) == 0
            && !pos.attackers_exist(opponent(side), king_sq-1, pos.get_pieces())) {
            *move_list++ = MoveEncoding::encode<MoveType::Castle>(king_sq, king_sq-2);
        }
    }

    return move_list;
}

/**
 * If king is in check, this assumes the move is an evasion.
 * Therefore, generate pseudo-legal evasion moves when king is in check.
 * Some info: https://chess.stackexchange.com/questions/15043/building-a-chess-using-magic-bitboard-how-do-i-know-if-the-movement-is-valid
 */
template<Color side>
inline bool is_legal_move(const Position& pos, Move move) {
    constexpr Color opp = opponent(side);
    Square from = MoveEncoding::from_sq(move);
    Square to = MoveEncoding::to_sq(move);
    MoveType move_type = MoveEncoding::move_type(move);

    // King move is legal if the destination square is not attacked
    // Handles castling also (other squares are checked during move generation)
    if (to_type(pos.get_piece_at(from)) == PieceType::King) {
        return !pos.attackers_exist(opp, to, pos.get_pieces() ^ MASK_SQUARE[+from]);
    }

    Square king_sq = lsb(pos.get_pieces(side, PieceType::King));

    // en passant special case: simulate occupancy after capture
    if (move_type == MoveType::EnPassant) {
        // If was an evasion generation move, we know the pawn is the only checker
        // If this was not an evasion generation, the king must not have been in check before the move
        // In either case, we only need to check sliding attackers after the en passant capture
        Square capture_square = (side == Color::White) ? (to + Shift::Down) : (to + Shift::Up);
        Bitboard occ = pos.get_pieces() ^ MASK_SQUARE[+from] ^ MASK_SQUARE[+capture_square] | MASK_SQUARE[+to];
        if (attacks_from<PieceType::Rook>(king_sq, occ) & (pos.get_pieces(opp, PieceType::Rook) | pos.get_pieces(opp, PieceType::Queen)))
            return false;
        if (attacks_from<PieceType::Bishop>(king_sq, occ) & (pos.get_pieces(opp, PieceType::Bishop) | pos.get_pieces(opp, PieceType::Queen)))
            return false;
        return true;
    }

    // Any other move is legal if the piece is not pinned or moves along the pin line
    // (again assuming evasion generation when in check)
    const bool is_pinned = (pos.get_king_blockers(side) & MASK_SQUARE[+from]) != 0ULL;
    const bool moves_on_line = (MASK_LINE[+from][+to] & MASK_SQUARE[+king_sq]) != 0ULL;
    return (!is_pinned || moves_on_line);
}

// Generates legal moves for the given side and generation type
template<GenerateType gen_type, Color side>
inline Move* generate_legal_moves_for_side(const Position& pos, Move* move_list) {
    static_assert(gen_type != GenerateType::Legal, "Invalid GenerateType for legal move generation (process generates pseudo-legal moves first)");

    // Generate pseudo-legal moves
    Move* cur = move_list;
    move_list = generate_moves_for_side<gen_type, side>(pos, move_list);

    // compress to legal
    for (Move* move_ptr = cur; move_ptr != move_list; ++move_ptr) {
        if (is_legal_move<side>(pos, *move_ptr)) {
            *cur++ = *move_ptr;
        }
    }

    return cur;
}

} // namespace


template<GenerateType gen_type>
Move* generate_moves(const Position& pos, Move* move_list) {
    // Evasion generation can only be used when in check
    // Captures and Quiets only when not in check
    if constexpr (gen_type == GenerateType::Evasions) {
        assert(pos.in_check());
    }
    else if constexpr (gen_type == GenerateType::Captures || gen_type == GenerateType::Quiets) {
        assert(!pos.in_check());
    }

    if constexpr (gen_type == GenerateType::Legal){
        if (pos.get_side_to_move() == Color::White) {
            if (pos.in_check()) {
                move_list = generate_legal_moves_for_side<GenerateType::Evasions, Color::White>(pos, move_list);
            }
            else {
                move_list = generate_legal_moves_for_side<GenerateType::PseudoLegal, Color::White>(pos, move_list);
            }
        } else {
            if (pos.in_check()) {
                move_list = generate_legal_moves_for_side<GenerateType::Evasions, Color::Black>(pos, move_list);
            }
            else {
                move_list = generate_legal_moves_for_side<GenerateType::PseudoLegal, Color::Black>(pos, move_list);
            }
        }
    }
    else { // Captures, Quiets, Evasions
        if (pos.get_side_to_move() == Color::White) {
            move_list = generate_legal_moves_for_side<gen_type, Color::White>(pos, move_list);
        } else {
            move_list = generate_legal_moves_for_side<gen_type, Color::Black>(pos, move_list);
        }
    }
    return move_list;
}

// Explicit template instantiations
template Move* generate_moves<GenerateType::Legal>(const Position&, Move*);
template Move* generate_moves<GenerateType::Evasions>(const Position&, Move*);
template Move* generate_moves<GenerateType::Captures>(const Position&, Move*);
template Move* generate_moves<GenerateType::Quiets>(const Position&, Move*);

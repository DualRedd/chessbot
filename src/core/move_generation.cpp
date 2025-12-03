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

} // namespace

Move* generate_pseudo_legal_moves(const Position& pos, Move* move_list) {
    if (pos.get_side_to_move() == Color::White) {
        move_list = generate_moves_for_side<Color::White>(pos, move_list);
    } else {
        move_list = generate_moves_for_side<Color::Black>(pos, move_list);
    }
    return move_list;
}


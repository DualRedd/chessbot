#pragma once

#include "../core/position.hpp"
#include "value_tables.hpp"

/**
 * @param pos the position to evaluate the move in
 * @param move the move to evaluate
 * @param min_eval minimum evaluation to consider the move favorable
 * @return Static Exchange Evaluation (SEE) value for the move in the position. True if SEE >= min_eval, else false.
 */
inline bool static_exchange_evaluation(const Position& pos, Move move, int32_t min_eval) {
    // https://www.chessprogramming.net/static-exchange-evaluation-in-chess/

    if (MoveEncoding::move_type(move) != MoveType::Normal) {
        // not implemented for non-normal moves, consider SEE = 0 for those
        return 0 >= min_eval;
    }

    const Square from = MoveEncoding::from_sq(move);
    const Square to = MoveEncoding::to_sq(move);

    assert(pos.get_piece_at(from) != Piece::None);
    assert(to_type(pos.get_piece_at(to)) != PieceType::King);

    int32_t see = PIECE_VALUES[+to_type(pos.get_piece_at(to))] - min_eval;
    if (see < 0) // side to move losing already
        return false;

    see = PIECE_VALUES[+to_type(pos.get_piece_at(from))] - see;
    if (see < 0) // opponent losing after recapture
        return true;

    Color side = pos.get_side_to_move();
    Bitboard occupied = pos.get_pieces() ^ MASK_SQUARE[+from];
    Bitboard all_attackers = pos.attackers(to, occupied);
    Bitboard pc;
    bool stm_winning = true;

    while (true) {
        side = opponent(side);
        all_attackers &= occupied;
        Bitboard side_attackers = all_attackers & pos.get_pieces(side);

        // Remove pinned pieces from attackers if pinners still exist
        if ((pos.get_pinners(opponent(side)) & occupied) != 0ULL) {
            side_attackers &= ~pos.get_king_blockers(side);
        }

        if (side_attackers == 0ULL)
            return stm_winning; // No more attackers
        stm_winning = !stm_winning;

        // Find least valuable attacker
        pc = pos.get_pieces(side, PieceType::Pawn) & side_attackers;
        if (pc != 0ULL) {
            see = PIECE_VALUES[+PieceType::Pawn] - see;
            if (see < 0)
                return stm_winning;

            occupied ^= MASK_SQUARE[+lsb(pc)];
            // add possible discovered attacks
            all_attackers |= attacks_from<PieceType::Bishop>(to, occupied)
                            & (pos.get_pieces(PieceType::Bishop) | pos.get_pieces(PieceType::Queen));
            continue;
        }

        pc = pos.get_pieces(side, PieceType::Knight) & side_attackers;
        if (pc != 0ULL) {
            see = PIECE_VALUES[+PieceType::Knight] - see;
            if (see < 0)
                return stm_winning;

            occupied ^= MASK_SQUARE[+lsb(pc)];
            // no possible discovered attacks
            continue;
        }

        pc = pos.get_pieces(side, PieceType::Bishop) & side_attackers;
        if (pc != 0ULL) {
            see = PIECE_VALUES[+PieceType::Bishop] - see;
            if (see < 0)
                return stm_winning;

            occupied ^= MASK_SQUARE[+lsb(pc)];
            // add possible discovered attacks
            all_attackers |= attacks_from<PieceType::Bishop>(to, occupied)
                            & (pos.get_pieces(PieceType::Bishop) | pos.get_pieces(PieceType::Queen));
            continue;
        }

        pc = pos.get_pieces(side, PieceType::Rook) & side_attackers;
        if (pc != 0ULL) {
            see = PIECE_VALUES[+PieceType::Rook] - see;
            if (see < 0)
                return stm_winning;

            occupied ^= MASK_SQUARE[+lsb(pc)];
            // add possible discovered attacks
            all_attackers |= attacks_from<PieceType::Rook>(to, occupied)
                            & (pos.get_pieces(PieceType::Rook) | pos.get_pieces(PieceType::Queen));
            continue;
        }

        pc = pos.get_pieces(side, PieceType::Queen) & side_attackers;
        if (pc != 0ULL) {
            see = PIECE_VALUES[+PieceType::Queen] - see;
            if (see < 0)
                return stm_winning;

            occupied ^= MASK_SQUARE[+lsb(pc)];
            // add possible discovered attacks
            all_attackers |= (attacks_from<PieceType::Bishop>(to, occupied)
                            | attacks_from<PieceType::Rook>(to, occupied))
                            & (pos.get_pieces(PieceType::Bishop) | pos.get_pieces(PieceType::Rook) | pos.get_pieces(PieceType::Queen));
            continue;
        }

        // Only king left
        // no possible discovered attacks
        // if attackers left for opponent, opponent wins
        if (all_attackers & ~pos.get_pieces(side))
            return !stm_winning;
        return stm_winning;
    }
}

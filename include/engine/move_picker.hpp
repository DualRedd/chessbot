#pragma once

#include <array>
#include "core/move_generation.hpp"
#include "killer_history.hpp"

/*
Normal moves
1. TT move
2. Good captures (SEE >= 0)
3. Quiets
4. Bad captures (SEE < 0)

Evasions
1. TT move
2. All other evasions

Quiescence
1. TT move
2. Good captures (SEE >= 0)
*/
enum class MovePickStage {
    // Normal moves
    TTMoveNormal,
    ScoreCaptures,
    GoodCaptures,
    FirstKillerMove,
    SecondKillerMove,
    ScoreQuiets,
    Quiets,
    BadCaptures,

    // Quiescence
    TTMoveQuiescence,
    ScoreQuiescenceCaptures,
    GoodQuiescenceCaptures,

    // Evasions
    TTMoveEvasion,
    ScoreEvasions,
    Evasions,
};

inline MovePickStage& operator++(MovePickStage& stage) {
    stage = static_cast<MovePickStage>(static_cast<int>(stage) + 1);
    return stage;
}

struct ScoredMove {
    Move move;
    int32_t score;
};

class MovePicker {
public:
    /**
     * Move picker for normal search.
     * @param position the position to pick moves from
     * @param tt_move the transposition table best move to prioritize (can be NO_MOVE)
     * @param killer_history pointer to the killer history for the current search
     * @param ply current search ply
     */
    MovePicker(const Position& position, const Move tt_move, KillerHistory* killer_history, int ply);

    /**
     * Move picker for quiescence search.
     * @param position the position to pick moves from
     * @param tt_move the transposition table best move to prioritize (can be NO_MOVE)
     */
    MovePicker(const Position& position, const Move tt_move);

    /**
     * @return Next best move according to the move ordering heuristics. Returns NO_MOVE if no moves are left.
     */
    Move next();

private:
    /**
     * Score the moves in the given move list according to various heuristics.
     * @tparam gen_type Type of moves in the move list (Captures, Quiets or Evasions)
     * @param move_list the move list to score
     * @param scored_list pointer to the beginning of the scored move list to fill
     * @return Pointer to one past the last scored move in the scored list.
     */
    template<GenerateType gen_type>
    ScoredMove* score_moves(const MoveList& move_list, ScoredMove* scored_list) const;

private:
    const Position& m_position;
    MovePickStage m_stage;
    Move m_tt_move;
    KillerHistory* m_killer_history;
    int m_ply;

    std::array<ScoredMove, MAX_MOVE_LIST_SIZE> m_scored_moves;
    ScoredMove *m_cur_begin, *m_cur_end;
    ScoredMove *m_bad_captures_begin, *m_bad_captures_end;
};

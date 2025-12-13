#include "engine/move_picker.hpp"

#include "engine/see.hpp"

static inline void insertion_sort(ScoredMove* begin, ScoredMove* end) {
    for (ScoredMove* it = begin + 1; it < end; ++it) {
        ScoredMove key = *it;
        ScoredMove* j = it;
        for (; j > begin && (j - 1)->score < key.score; --j) {
            *j = *(j - 1);
        }
        *j = key;
    }
}

MovePicker::MovePicker(const Position& position, const Move tt_move, KillerHistory* killer_history, int ply)
  : m_position(position),
    m_tt_move(tt_move),
    m_killer_history(killer_history),
    m_ply(ply),
    m_scored_moves{},
    m_cur_begin(m_scored_moves.data())
{
    if (m_position.in_check())
        m_stage = MovePickStage::TTMoveEvasion;
    else
        m_stage = MovePickStage::TTMoveNormal;

    // Skip TT move stage if TT move is illegal
    // zobrist hash collision for example can result in a illegal move retrieved from a TT
    if (m_tt_move == NO_MOVE || !test_legality(m_position, m_tt_move))
        ++m_stage;
}

MovePicker::MovePicker(const Position& position, const Move tt_move) 
  : m_position(position),
    m_tt_move(tt_move),
    m_scored_moves{},
    m_cur_begin(m_scored_moves.data())
{
    // TT move not used in quiescence search
    if (m_position.in_check())
        m_stage = MovePickStage::TTMoveEvasion;
    else {
        m_stage = MovePickStage::TTMoveQuiescence;
        // Check if TT move is a capture or queen promotion
        // If not, it is not included in quiescence search
        if (m_tt_move != NO_MOVE && (m_position.to_capture(m_tt_move) == PieceType::None
            || (MoveEncoding::move_type(m_tt_move) == MoveType::Promotion && MoveEncoding::promo(m_tt_move) != PieceType::Queen))) {
            m_tt_move = NO_MOVE;
        }
    }

    // Skip TT move stage if TT move is illegal
    if (m_tt_move == NO_MOVE || !test_legality(m_position, m_tt_move))
        ++m_stage;
}

Move MovePicker::next() {
    switch (m_stage) {
        case MovePickStage::TTMoveNormal:
        case MovePickStage::TTMoveEvasion:
        case MovePickStage::TTMoveQuiescence: {
            ++m_stage;
            return m_tt_move;
        }

        case MovePickStage::ScoreCaptures:
        case MovePickStage::ScoreQuiescenceCaptures: {
            MoveList captures;
            captures.generate<GenerateType::Captures>(m_position);
            m_cur_end = score_moves<GenerateType::Captures>(captures, m_cur_begin);
            m_bad_captures_begin = m_bad_captures_end = m_cur_begin;
            insertion_sort(m_cur_begin, m_cur_end);
            ++m_stage;
            [[fallthrough]];
        }

        case MovePickStage::GoodCaptures:
        case MovePickStage::GoodQuiescenceCaptures: {
            while (m_cur_begin < m_cur_end) {
                if (m_cur_begin->move == m_tt_move) {
                    ++m_cur_begin;
                    continue;
                }
                if (static_exchange_evaluation(m_position, m_cur_begin->move, 0))
                    return (m_cur_begin++)->move;
                else {
                    // Move to front (preserver order for bad captures)
                    std::swap(*(m_bad_captures_end++), *(m_cur_begin++));
                }
            }
            if (m_stage == MovePickStage::GoodQuiescenceCaptures)
                return NO_MOVE;

            ++m_stage;
            [[fallthrough]];
        }

        case MovePickStage::FirstKillerMove: {
            Move killer = m_killer_history->first(m_ply);
            if (++m_stage; killer != NO_MOVE && killer != m_tt_move && test_legality(m_position, killer))
                return killer;
            [[fallthrough]];
        }

        case MovePickStage::SecondKillerMove: {
            Move killer = m_killer_history->second(m_ply);
            if (++m_stage; killer != NO_MOVE && killer != m_tt_move && test_legality(m_position, killer))
                return killer;
            [[fallthrough]];
        }

        case MovePickStage::ScoreQuiets: {
            MoveList quiets;
            quiets.generate<GenerateType::Quiets>(m_position);
            m_cur_end = score_moves<GenerateType::Quiets>(quiets, m_cur_begin);
            insertion_sort(m_cur_begin, m_cur_end);
            ++m_stage;
            [[fallthrough]];
        }

        case MovePickStage::Quiets: {
            while (m_cur_begin < m_cur_end) {
                if (m_cur_begin->move == m_tt_move
                    || m_cur_begin->move == m_killer_history->first(m_ply)
                    || m_cur_begin->move == m_killer_history->second(m_ply)) {
                    ++m_cur_begin;
                    continue;
                }
                return (m_cur_begin++)->move;
            }
            ++m_stage;
            [[fallthrough]];
        }
        
        case MovePickStage::BadCaptures: {
            while (m_bad_captures_begin < m_bad_captures_end) {
                if (m_bad_captures_begin->move == m_tt_move) {
                    ++m_bad_captures_begin;
                    continue;
                }
                return (m_bad_captures_begin++)->move;
            }
            return NO_MOVE;
        }
        
        case MovePickStage::ScoreEvasions: {
            MoveList evasions;
            evasions.generate<GenerateType::Evasions>(m_position);
            m_cur_end = score_moves<GenerateType::Evasions>(evasions, m_cur_begin);
            insertion_sort(m_cur_begin, m_cur_end);
            ++m_stage;
            [[fallthrough]];
        }

        case MovePickStage::Evasions: {
            while (m_cur_begin < m_cur_end) {
                if (m_cur_begin->move == m_tt_move) {
                    ++m_cur_begin;
                    continue;
                }
                return (m_cur_begin++)->move;
            }
            return NO_MOVE;
        }

        assert(false); // should not reach here
    }
    return NO_MOVE; // suppress compiler warning
};

template<GenerateType gen_type>
ScoredMove* MovePicker::score_moves(const MoveList& move_list, ScoredMove* scored_list) const {
    static_assert(gen_type == GenerateType::Captures || gen_type == GenerateType::Quiets || gen_type == GenerateType::Evasions);

    for (Move move : move_list) {
        ScoredMove& cur = *scored_list++;
        cur.move = move;
        cur.score = 0;

        if constexpr (gen_type == GenerateType::Captures) {
            // Captures, least valuable attacker capturing most valuable victim
            PieceType captured = m_position.to_capture(cur.move);
            PieceType attacker = m_position.to_moved(cur.move);
            cur.score += PIECE_VALUES[+captured] - PIECE_VALUES[+attacker];

            // Check queen promo
            if (MoveEncoding::move_type(cur.move) == MoveType::Promotion)
                cur.score += PIECE_VALUES[+PieceType::Queen] * 100;
        }
        else if constexpr (gen_type == GenerateType::Evasions) {
            // Prefer captures
            PieceType captured = m_position.to_capture(cur.move);
            if (captured != PieceType::None)
                cur.score += 1'000;
        }
        else { // Quiets
            // Prefer non-pawn non-king moves
            PieceType mover = m_position.to_moved(cur.move);
            cur.score += (mover < PieceType::Pawn) ? 1'000 : 0;

            // Prefer moves that move towards center
            Square to = MoveEncoding::to_sq(cur.move);
            int center_dist = std::abs(3 - file_of(to)) + std::abs(3 - rank_of(to));
            cur.score += (10 - center_dist);
        }
    }

    return scored_list;
}

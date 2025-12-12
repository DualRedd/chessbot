#include "engine/move_picker.hpp"

#include <algorithm>
#include "engine/see.hpp"


MovePicker::MovePicker(const Position& position, const Move tt_move, bool quiescence_search)
  : m_position(position),
    m_tt_move(tt_move),
    m_scored_moves{},
    m_cur_begin(m_scored_moves.data()),
    m_cur_end(nullptr),
    m_bad_captures_begin(nullptr),
    m_bad_captures_end(nullptr)
{
    if (m_position.in_check())
        m_stage = MovePickStage::TTMoveEvasion;
    else if (!quiescence_search)
        m_stage = MovePickStage::TTMoveNormal;
    else {
        m_stage = MovePickStage::TTMoveQuiescence;
        if (m_tt_move != NULL_MOVE && (m_position.to_capture(m_tt_move) == PieceType::None
            || (MoveEncoding::move_type(m_tt_move) == MoveType::Promotion && MoveEncoding::promo(m_tt_move) != PieceType::Queen))) {
            // TT move is not a capture or queen promot (not included in quiescence search)
            m_tt_move = NULL_MOVE;
        }
    }

    // Skip TT move stage if no valid TT move
    // zobrist hash collision for example can result in a illegal move retrieved from the transposition table (quite rare)
    if (m_tt_move == NULL_MOVE || !test_legality(m_position, m_tt_move))
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
            m_bad_captures_end = m_cur_end;
            std::stable_sort(m_cur_begin, m_cur_end, [](const ScoredMove& a, const ScoredMove& b) {
                return a.score > b.score;
            });
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
                    std::swap(*m_cur_begin, *(--m_cur_end));
                }
            }
            if (m_stage == MovePickStage::GoodQuiescenceCaptures)
                return NULL_MOVE;

            ++m_stage;
            m_bad_captures_begin = m_cur_end;
            m_cur_begin = m_bad_captures_end;
            [[fallthrough]];
        }

        case MovePickStage::ScoreQuiets: {
            MoveList quiets;
            quiets.generate<GenerateType::Quiets>(m_position);
            m_cur_end = score_moves<GenerateType::Quiets>(quiets, m_cur_begin);
            std::stable_sort(m_cur_begin, m_cur_end, [](const ScoredMove& a, const ScoredMove& b) {
                return a.score > b.score;
            });
            ++m_stage;
            [[fallthrough]];
        }

        case MovePickStage::Quiets: {
            while (m_cur_begin < m_cur_end) {
                if (m_cur_begin->move == m_tt_move) {
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
            return NULL_MOVE;
        }
        
        case MovePickStage::ScoreEvasions: {
            MoveList evasions;
            evasions.generate<GenerateType::Evasions>(m_position);
            m_cur_end = score_moves<GenerateType::Evasions>(evasions, m_cur_begin);
            std::stable_sort(m_cur_begin, m_cur_end, [](const ScoredMove& a, const ScoredMove& b) {
                return a.score > b.score;
            });
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
            return NULL_MOVE;
        }

        assert(false); // should not reach here
    }
    return NULL_MOVE; // suppress compiler warning
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

// Explicit template instantiations
template ScoredMove* MovePicker::score_moves<GenerateType::Captures>(const MoveList& move_list, ScoredMove* scored_list) const;
template ScoredMove* MovePicker::score_moves<GenerateType::Quiets>(const MoveList& move_list, ScoredMove* scored_list) const;
template ScoredMove* MovePicker::score_moves<GenerateType::Evasions>(const MoveList& move_list, ScoredMove* scored_list) const;

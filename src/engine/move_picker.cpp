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

MovePicker::MovePicker(const Position& position, int ply, const Move tt_move, KillerHistory* killer_history, MoveHistory* move_history)
  : m_position(position),
    m_ply(ply),
    m_tt_move(tt_move),
    m_killer_history(killer_history),
    m_move_history(move_history),
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

MovePicker::MovePicker(const Position& position, const Move tt_move, MoveHistory* move_history) 
  : m_position(position),
    m_tt_move(tt_move),
    m_move_history(move_history),
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

void MovePicker::skip_quiets() {
    m_skip_quiets = true;
}

void MovePicker::repick_quiets() {
    assert(m_quiets_begin != nullptr && m_quiets_end != nullptr);
    m_cur_begin = m_quiets_begin;
    m_cur_end = m_quiets_end;
    m_stage = MovePickStage::Quiets;
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
        }
        [[fallthrough]];

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
                    // Move to front (preserve order)
                    std::swap(*(m_bad_captures_end++), *(m_cur_begin++));
                }
            }
            if (m_stage == MovePickStage::GoodQuiescenceCaptures)
                return NO_MOVE;

            ++m_stage;
        }
        [[fallthrough]];

        case MovePickStage::FirstKillerMove: {
            Move killer = m_killer_history->first(m_ply);
            if (++m_stage; killer != NO_MOVE && killer != m_tt_move && test_legality(m_position, killer))
                return killer;
        }
        [[fallthrough]];

        case MovePickStage::SecondKillerMove: {
            Move killer = m_killer_history->second(m_ply);
            if (++m_stage; killer != NO_MOVE && killer != m_tt_move && test_legality(m_position, killer))
                return killer;
        }
        [[fallthrough]];

        case MovePickStage::ScoreQuiets: if (!m_skip_quiets) {
            MoveList quiets;
            quiets.generate<GenerateType::Quiets>(m_position);
            m_cur_end = score_moves<GenerateType::Quiets>(quiets, m_cur_begin);
            insertion_sort(m_cur_begin, m_cur_end);
            m_quiets_begin = m_cur_begin;
            m_quiets_end = m_cur_end;
            ++m_stage;
        }
        [[fallthrough]];

        case MovePickStage::Quiets: if (!m_skip_quiets) {
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
        }
        [[fallthrough]];
        
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
        }
        [[fallthrough]];

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

    // Calculate threats by lesser pieces
    Bitboard threatened_by_lesser[6] = {0, 0, 0, 0, 0, 0};
    if constexpr (gen_type == GenerateType::Quiets) {
        const Color opp = opponent(m_position.get_side_to_move());
        const Bitboard occupied = m_position.get_pieces();

        if (opp == Color::White)
            threatened_by_lesser[+PieceType::Knight] = pawn_attacks<Color::White>(m_position.get_pieces(Color::White, PieceType::Pawn));
        else
            threatened_by_lesser[+PieceType::Knight] = pawn_attacks<Color::Black>(m_position.get_pieces(Color::Black, PieceType::Pawn));

        threatened_by_lesser[+PieceType::Bishop] = threatened_by_lesser[+PieceType::Knight];
        threatened_by_lesser[+PieceType::Rook]   = threatened_by_lesser[+PieceType::Knight]
                                                 | piece_attacks<PieceType::Knight>(m_position.get_pieces(opp, PieceType::Knight), occupied)
                                                 | piece_attacks<PieceType::Bishop>(m_position.get_pieces(opp, PieceType::Bishop), occupied);
        threatened_by_lesser[+PieceType::Queen]  = threatened_by_lesser[+PieceType::Rook]
                                                 | piece_attacks<PieceType::Rook>(m_position.get_pieces(opp, PieceType::Rook), occupied);
    }

    for (Move move : move_list) {
        ScoredMove& cur = *scored_list++;
        cur.move = move;
        cur.score = 0;

        if constexpr (gen_type == GenerateType::Captures) {
            // Captures, least valuable attacker capturing most valuable victim
            PieceType captured = m_position.to_capture(cur.move);
            PieceType attacker = m_position.to_moved(cur.move);
            cur.score += PIECE_VALUES[+captured] - PIECE_VALUES[+attacker];

            // queen promo
            if (MoveEncoding::move_type(cur.move) == MoveType::Promotion)
                cur.score += PIECE_VALUES[+PieceType::Queen] << 2;
        }
        else if constexpr (gen_type == GenerateType::Evasions) {
            // Prefer captures
            PieceType captured = m_position.to_capture(cur.move);
            if (captured != PieceType::None) {
                cur.score += PIECE_VALUES[+captured] + MOVE_HISTORY_MAX_VALUE;
            }
            else {
                // History heuristic for non-captures
                cur.score += m_move_history->get(m_position, cur.move);
            }
        }
        else { // Quiets
            const PieceType mover = m_position.to_moved(cur.move);
            const Square from = MoveEncoding::to_sq(cur.move);
            const Square to = MoveEncoding::to_sq(cur.move);

            // Prefer moves that don't move into lesser piece threats
            if (threatened_by_lesser[+mover] & MASK_SQUARE[+to])
                cur.score -= 29;

            // Prefer moves that evade lesser piece threats
            if (threatened_by_lesser[+mover] & MASK_SQUARE[+from])
                cur.score += 30;

            // History heuristic
            cur.score += m_move_history->get(m_position, cur.move);
        }
    }

    return scored_list;
}

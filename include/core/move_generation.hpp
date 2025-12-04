#pragma once

#include "position.hpp"
#include<iostream> // DEBUG
Move* generate_pseudo_legal_moves(const Position& pos, Move* move_list);

class MoveList {
public:
    MoveList() : m_count(0) {}

    void generate_pseudo(const Position& pos) {
        Move* end = generate_pseudo_legal_moves(pos, m_moves.begin());
        m_count = end - m_moves.begin();
    }

    void generate_legal(const Position& pos) {
        m_count = 0;
        Move* end = generate_pseudo_legal_moves(pos, m_moves.begin());
        for (Move* move_ptr = m_moves.begin(); move_ptr != end; ++move_ptr) {
            if (pos.is_legal_move(*move_ptr)) {
                m_moves[m_count++] = *move_ptr;
            }
        }
    }

    /*void generate_legal(const Position& pos) {
        Position copy(pos, false);
        m_count = 0;
        Move* end = generate_pseudo_legal_moves(pos, m_moves.begin());

        for (Move* move_ptr = m_moves.begin(); move_ptr != end; ++move_ptr) {
            Move move = *move_ptr;
            copy.make_move(move);
            if (!copy.in_check(pos.get_side_to_move())) {
                m_moves[m_count++] = *move_ptr;
            }
            copy.undo_move();
        }
    }*/

    size_t count() const {
        return m_count;
    }

    Move operator[](size_t index) const {
        return m_moves[index];
    }

    Move* begin() {
        return m_moves.begin();
    }

    Move* end() {
        return m_moves.begin() + m_count;
    }

private:
    static constexpr int MAX_PSEUDO_LEGAL_MOVES = 280;
    std::array<Move, MAX_PSEUDO_LEGAL_MOVES> m_moves;
    size_t m_count;
};

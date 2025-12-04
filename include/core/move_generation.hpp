#pragma once

#include "position.hpp"

Move* generate_pseudo_legal_moves(const Position& pos, Move* move_list);

Move* generate_legal_moves(const Position& pos, Move* move_list);

class MoveList {
public:
    MoveList() : m_count(0) {}

    void generate_pseudo(const Position& pos) {
        Move* end = generate_pseudo_legal_moves(pos, m_moves.begin());
        m_count = end - m_moves.begin();
    }

    /*void generate_legal(const Position& pos) {
        Move* end = generate_legal_moves(pos, m_moves.begin());
        m_count = end - m_moves.begin();
    }*/

    void generate_legal(const Position& pos) {
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
    }

    size_t count() const {
        return m_count;
    }

    Move& operator[](size_t index) { return m_moves[index]; }
    const Move& operator[](size_t index) const { return m_moves[index]; }

    Move* begin() {
        return m_moves.begin();
    }

    Move* end() {
        return m_moves.begin() + m_count;
    }

public:
    static constexpr int MAX_SIZE = 280; // Upper limit for pseudo-legal moves in any position
private:
    std::array<Move, MAX_SIZE> m_moves;
    size_t m_count;
};

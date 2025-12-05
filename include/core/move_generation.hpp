#pragma once

#include <array>
#include "position.hpp"

constexpr int MAX_MOVE_LIST_SIZE = 256; // Upper limit for pseudo-legal moves in any position

/**
 * Legal: Generate only legal moves
 * PseudoLegal: Generate all pseudo-legal moves
 * Evasions: Generate pseudo-legal moves that could evade check
 * Captures: Generate pseudo-legal capture moves
 * Quiets: Generate pseudo-legal non-capture moves
 */
enum class GenerateType : uint8_t { Legal, PseudoLegal, Evasions, Captures, Quiets,  };

/**
 * Generate moves of specified type for the given position.
 * @param pos Position to generate moves for.
 * @param move_list Pointer to the beginning of the move list array to fill.
 * @return Pointer to one past the last move written.
 */
template<GenerateType gen_type>
Move* generate_moves(const Position& pos, Move* move_list);

/**
 * Simple wrapper class to store and generate moves easily.
 */
class MoveList {
public:
    MoveList() : m_count(0) {}

    template<GenerateType gen_type>
    void generate(const Position& pos) {
        Move* end = generate_moves<gen_type>(pos, m_moves.begin());
        m_count = end - m_moves.begin();
    }

    Move& operator[](size_t index) { return m_moves[index]; }
    const Move& operator[](size_t index) const { return m_moves[index]; }

    size_t count() const { return m_count; }
    Move* begin() { return m_moves.begin(); }
    Move* end() { return m_moves.begin() + m_count; }

private:
    std::array<Move, MAX_MOVE_LIST_SIZE> m_moves;
    size_t m_count;
};

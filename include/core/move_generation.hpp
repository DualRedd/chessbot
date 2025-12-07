#pragma once

#include <array>
#include "position.hpp"

constexpr int MAX_MOVE_LIST_SIZE = 256; // Upper limit for pseudo-legal moves in any position

/**
 * Legal: legal moves
 * PseudoLegal: pseudo-legal moves
 * Evasions: legal check evasions
 * Captures: legal capture moves (and queen promotions)
 * Quiets: legal non-capture moves (excluding queen promotions)
 */
enum class GenerateType : uint8_t { Legal, PseudoLegal, Evasions, Captures, Quiets };

/**
 * Generate legal moves of specified type for the given position.
 * @note Pseudo-legal move generation is not supported by this function.
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

    /**
     * Fill the move list with legal moves of the specified type for the given position.
     * @note Pseudo-legal move generation is not supported by this function.
     * @param gen_type Type of moves to generate.
     * @param pos Position to generate moves for.
     * @warning Evasions can only be generated when the side to move is in check. Captures and Quiets only when not in check.
     * No runtime checks apart from asserts are done.
     */
    template<GenerateType gen_type>
    void generate(const Position& pos) {
        Move* end = generate_moves<gen_type>(pos, m_moves.data());
        m_count = end - m_moves.data();
    }

    Move& operator[](size_t index) { return m_moves[index]; }
    const Move& operator[](size_t index) const { return m_moves[index]; }

    size_t count() const { return m_count; }
    Move* begin() { return m_moves.data(); }
    Move* end() { return m_moves.data() + m_count; }

private:
    std::array<Move, MAX_MOVE_LIST_SIZE> m_moves;
    size_t m_count;
};

#pragma once

#include <cstdint>
#include <array>
#include <algorithm>

#include "core/types.hpp"
#include "core/position.hpp"

constexpr int32_t KILLER_HISTORY_MAX_PLIES = 256;
constexpr int32_t MOVE_HISTORY_MAX_VALUE = 45'000;

class KillerHistory {
public:
    KillerHistory() { reset(); }

    void reset() {
        for (int ply = 0; ply < KILLER_HISTORY_MAX_PLIES; ++ply) {
            m_killers[0][ply] = m_killers[1][ply] = NO_MOVE;
        }
    }

    void store(Move move, int ply) {
        assert(ply >= 0 && ply < KILLER_HISTORY_MAX_PLIES);

        if (m_killers[0][ply] != move) {
            m_killers[1][ply] = m_killers[0][ply];
            m_killers[0][ply] = move;
        }
    }

    Move first(int ply) const {
        assert(ply >= 0 && ply < KILLER_HISTORY_MAX_PLIES);
        return m_killers[0][ply];
    }

    Move second(int ply) const {
        assert(ply >= 0 && ply < KILLER_HISTORY_MAX_PLIES);
        return m_killers[1][ply];
    }

private:
    Move m_killers[2][KILLER_HISTORY_MAX_PLIES];
};

class MoveHistory {
public:
    MoveHistory() { reset(); }

    void reset() {
        for (int piece = 0; piece < 14; ++piece) {
            for (int to = 0; to < 64; ++to)
                m_history[piece][to] = 0;
        }
    }

    void update(const Position& pos, Move move, int32_t bonus) {
        const Square from = MoveEncoding::from_sq(move);
        const Square to = MoveEncoding::to_sq(move);
        const Piece piece = pos.get_piece_at(from);

        assert(+piece >= 0 && +piece < 14);

        // history gravity formula
        int32_t bonus_clamped = std::clamp(bonus, -MOVE_HISTORY_MAX_VALUE, MOVE_HISTORY_MAX_VALUE);
        m_history[+piece][+to] += bonus_clamped - m_history[+piece][+to] * abs(bonus_clamped) / MOVE_HISTORY_MAX_VALUE;
    }

    int32_t get(const Position& pos, Move move) const {
        const Square from = MoveEncoding::from_sq(move);
        const Square to = MoveEncoding::to_sq(move);
        const Piece piece = pos.get_piece_at(from);

        assert(+piece >= 0 && +piece < 14);

        return m_history[+piece][+to];
    }

private:
    int32_t m_history[14][64]; // [piece][to]
};

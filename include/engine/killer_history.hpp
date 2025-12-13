#pragma once

#include <cstdint>
#include <array>

#include "core/types.hpp"

constexpr int32_t KILLER_HISTORY_MAX_PLIES = 256;

class KillerHistory {
public:
    KillerHistory() = default;

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

#pragma once

#include "board.hpp"

/**
 * Abstract base class representing an AI player
 */
class AIPlayer {
public:
    AIPlayer() = default;
    virtual ~AIPlayer() = default;

    /**
     * Request a move from this player.
     * @param board board representation.
     * @return A move response.
     */
    virtual Move getMove(const Board& board) = 0;
};
#pragma once

#include <atomic>
#include <memory>

#include "types.hpp"

/**
 * Represents the state of an asynchronous move request.
 */
struct AsyncMoveTask {
    std::atomic_bool done{false};
    std::exception_ptr error;
    UCI result;
};

/**
 * Base class representing an AI player.
 */
class AIPlayer {
public:
    AIPlayer() = default;
    virtual ~AIPlayer() = default;

    /**
     * Request a move from this player.
     * @param board board FEN representation.
     * @return UCI move response.
     */
    virtual UCI getMove(const FEN& fen) = 0;

    /**
     * Request a move from this player asynchronously.
     * @param board board FEN representation.
     * @return AsyncMoveTask indicating response status.
     */
    std::shared_ptr<AsyncMoveTask> getMoveAsync(const FEN& fen);
};

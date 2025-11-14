#pragma once

#include <atomic>
#include <memory>

#include "standards.hpp"

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
     * Set the board state.
     * @param board board FEN representation.
     * @throw std::invalid_argument if the board could not be set
     */
    virtual void set_board(const FEN& fen) = 0;

    /**
     * Apply a move to the current board.
     * @param move UCI move string
     * @throw std::invalid_argument if the move could not be applied
     */
    virtual void apply_move(const UCI& move) = 0;

    /**
     * Undo a move on the current board.
     * @throw std::invalid_argument if a move could not be undone
     */
    virtual void undo_move() = 0;

    /**
     * Compute a move on the current board.
     * @return UCI move response.
     */
    virtual UCI compute_move() = 0;

    /**
     * Compute a move on the current board asynchronously.
     * @return AsyncMoveTask indicating response status.
     */
    std::shared_ptr<AsyncMoveTask> compute_move_async();
};

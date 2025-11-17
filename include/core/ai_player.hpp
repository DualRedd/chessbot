#pragma once

#include <atomic>
#include <memory>

#include "standards.hpp"

/**
 * Represents the state of an asynchronous move request.
 */
struct AsyncMoveCompute {
    std::atomic_bool done{false};
    std::exception_ptr error;
    UCI result;
};

/**
 * Base class representing an AI player.
 */
class AIPlayer {
public:
    AIPlayer();
    virtual ~AIPlayer();

    /**
     * Inform the AI about a new board state.
     * @param board board FEN representation.
     * @throw std::runtime_error if the AI is currently computing a move.
     * @throw std::invalid_argument if the board could not be set.
     */
    void set_board(const FEN& fen);

    /**
     * Inform the AI about a move on the board.
     * @param move UCI move string
     * @throw std::runtime_error if the AI is currently computing a move.
     * @throw std::invalid_argument if the move could not be applied
     */
    void apply_move(const UCI& move);

    /**
     * Inform the AI about a move being undone on the board.
     * @throw std::runtime_error if the AI is currently computing a move.
     * @throw std::invalid_argument if a move could not be undone
     */
    void undo_move();

    /**
     * Compute a move on the current board.
     * @return UCI move response.
     * @throw std::runtime_error if the AI is currently computing a move.
     * @throw std::invalid_argument if a a move could not be calculated
     */
    UCI compute_move();

    /**
     * Compute a move on the current board asynchronously.
     * @return AsyncMoveCompute indicating response status. 
     * @note AsyncMoveCompute::done will be set to true after an error occured or the result was succesfully computed.
     */
    std::shared_ptr<AsyncMoveCompute> compute_move_async();

    /**
     * Request the current move compute to stop (if there is one).
     * This can be used to stop async move computes.
     * @warning This does not quarantee when the AI will stop. It is up to the implementation.
     * @note Use is_computing() to track when the move compute has stopped. 
     */
    void request_stop();

    /**
     * @return True if this AI is computing a move, else false.
     */
    bool is_computing() const;

protected:
    /**
     * @return True if the current compute has been requested to stop, else false.
     */
    bool _stop_requested() const;

    /**
     * Implements internal state update when a new board is set.
     * @param board board FEN representation.
     * @throw std::invalid_argument if the move could not be applied
     * @note It is quaranteed that only one of _set_board(), _apply_move(), _undo_move()
     *      and _compute_move() can be called at a time from any number of threads.
     */
    virtual void _set_board(const FEN& fen) = 0;

    /**
     * Implements internal state update when a move is applied.
     * @param move UCI move string
     * @throw std::invalid_argument if the move could not be applied
     * @note It is quaranteed that only one of _set_board(), _apply_move(), _undo_move()
     *      and _compute_move() can be called at a time from any number of threads.
     */
    virtual void _apply_move(const UCI& move) = 0;

    /**
     * Implements internal state update when a move is undone.
     * @throw std::invalid_argument if a move could not be undone
     * @note It is quaranteed that only one of _set_board(), _apply_move(), _undo_move()
     *      and _compute_move() can be called at a time from any number of threads.
     */
    virtual void _undo_move() = 0;

    /**
     * Implements computing a move on the current board state.
     * @attention Implementers are encouraged to handle early return when _stop_requested() to better support async move computation.
     * @return UCI move response.
     * @throw std::invalid_argument if a a move could not be calculated
     * @note It is quaranteed that only one of _set_board(), _apply_move(), _undo_move()
     *      and _compute_move() can be called at a time from any number of threads.
     */
    virtual UCI _compute_move() = 0;

private:
    std::atomic_bool m_computing = false;
    std::atomic_bool m_stop_requested = false;
};

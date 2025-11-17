#include "core/ai_player.hpp"

#include <thread>
#include <stdexcept>

AIPlayer::AIPlayer() = default;
AIPlayer::~AIPlayer() = default;

void AIPlayer::set_board(const FEN& fen) {
    bool expected = false;
    if (!m_computing.compare_exchange_strong(expected, true)) {
        throw std::runtime_error("AIPlayer::set_board() - too many concurrent requests!");
    }
    
    // Safe call (computing flag always cleared)
    try {
        _set_board(fen);
    }
    catch (...) {
        m_computing.store(false);
        throw;
    }
    m_computing.store(false);
}

void AIPlayer::apply_move(const UCI& move) {
    bool expected = false;
    if (!m_computing.compare_exchange_strong(expected, true)) {
        throw std::runtime_error("AIPlayer::apply_move() - too many concurrent requests!");
    }

    // Safe call (computing flag always cleared)
    try {
        _apply_move(move);
    }
    catch (...) {
        m_computing.store(false);
        throw;
    }
    m_computing.store(false);
}

void AIPlayer::undo_move() {
    bool expected = false;
    if (!m_computing.compare_exchange_strong(expected, true)) {
        throw std::runtime_error("AIPlayer::undo_move() - too many concurrent requests!");
    }

    // Safe call (computing flag always cleared)
    try {
        _undo_move();
    }
    catch (...) {
        m_computing.store(false);
        throw;
    }
    m_computing.store(false);
}

UCI AIPlayer::compute_move() {
    bool expected = false;
    if (!m_computing.compare_exchange_strong(expected, true)) {
        throw std::runtime_error("AIPlayer::compute_move() - too many concurrent requests!");
    }

    // Reset stop signal
    m_stop_requested.store(false);

    // Compute move safely (computing flag always cleared)
    UCI move;
    try {
        move = _compute_move();
    }
    catch (...) {
        m_computing.store(false);
        throw;
    }
    m_computing.store(false);

    return move;
}

std::shared_ptr<AsyncMoveCompute> AIPlayer::compute_move_async() {
    bool expected = false;
    if (!m_computing.compare_exchange_strong(expected, true)) {
        throw std::runtime_error("AIPlayer::compute_move_async() - too many concurrent requests!");
    }

    // Reset stop signal
    m_stop_requested.store(false);

    // Create shared compute state tracker
    auto task = std::make_shared<AsyncMoveCompute>();

    std::thread([this, task]() {
        try {
            task->result = _compute_move();
        }
        catch (...) {
            task->error = std::current_exception();
        }
        task->done = true;
        m_computing.store(false);
    }).detach();

    return task;
}

void AIPlayer::request_stop() {
    m_stop_requested.store(true);
}

bool AIPlayer::is_computing() const {
    return m_computing.load(); 
}

bool AIPlayer::_stop_requested() const {
    return m_stop_requested.load();
}

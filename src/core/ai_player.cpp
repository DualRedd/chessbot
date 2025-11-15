#include "core/ai_player.hpp"

#include <thread>

AIPlayer::AIPlayer() = default;
AIPlayer::~AIPlayer() = default;

std::shared_ptr<AsyncMoveCompute> AIPlayer::compute_move_async() {
    auto task = std::make_shared<AsyncMoveCompute>();

    std::thread([this, task]() {
        try {
            task->result = compute_move();
        }
        catch (...) {
            task->error = std::current_exception();
        }
        task->done = true;
    }).detach();

    return task;
}

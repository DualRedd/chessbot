#include "core/ai_player.hpp"

#include <thread>

std::shared_ptr<AsyncMoveTask> AIPlayer::compute_move_async() {
    auto task = std::make_shared<AsyncMoveTask>();

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

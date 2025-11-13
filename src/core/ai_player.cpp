#include "core/ai_player.hpp"

std::shared_ptr<AsyncMoveTask> AIPlayer::getMoveAsync(const FEN& fen) {
    auto task = std::make_shared<AsyncMoveTask>();

    std::thread([this, fen, task]() {
        try {
            task->result = getMove(fen);
        }
        catch (...) {
            task->error = std::current_exception();
        }
        task->done = true;
    }).detach();

    return task;
}

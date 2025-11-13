#include "ai/ai_random.hpp"

#include <random>

void registerRandomAI() {
    auto createRandomAI = [](const std::vector<ConfigField>& cfg) {
        return std::make_unique<RandomAI>();
    };

    AIRegistry::registerAI("Random", {}, createRandomAI);
}


UCI RandomAI::getMove(const FEN& fen) {
    Board board(fen);
    auto moves = board.generate_legal_moves();

    if (moves.empty()) {
        throw std::runtime_error("RandomAI::getMove() - no legal moves!");
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);

    return MoveEncoding::to_uci(moves[dist(rng)]);
}
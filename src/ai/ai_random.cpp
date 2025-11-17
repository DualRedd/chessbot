#include "ai/ai_random.hpp"

#include <random>

void registerRandomAI() {
    auto createRandomAI = [](const std::vector<ConfigField>& cfg) {
        return std::make_unique<RandomAI>();
    };

    AIRegistry::registerAI("Random", {}, createRandomAI);
}

void RandomAI::_set_board(const FEN& fen) {
    m_board.set_from_fen(fen);
}

void RandomAI::_apply_move(const UCI& uci_move) {
    Move move = m_board.move_from_uci(uci_move);
    auto legal_moves = m_board.generate_legal_moves();
    if(std::find(legal_moves.begin(), legal_moves.end(), move) == legal_moves.end()){
        throw std::invalid_argument("RandomAI::apply_move() - illegal move!");
    }
    m_board.make_move(move);
}

void RandomAI::_undo_move() {
    if(!m_board.undo_move()){
        throw std::invalid_argument("RandomAI::undo_move() - no previous move!");
    }
}

UCI RandomAI::_compute_move() {
    auto moves = m_board.generate_legal_moves();

    if (moves.empty()) {
        throw std::runtime_error("RandomAI::compute_move() - no legal moves!");
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);

    return MoveEncoding::to_uci(moves[dist(rng)]);
}

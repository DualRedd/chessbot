#include "ai/ai_random.hpp"

void registerRandomAI() {
    auto createRandomAI = [](const std::vector<ConfigField>& cfg) {
        return std::make_unique<RandomAI>(cfg);
    };

    std::vector<ConfigField> cfg = {
        {"useseed", "Use seed", FieldType::Bool, false},
        {"rngseed", "Seed", FieldType::Int, 0}
    };

    AIRegistry::registerAI("Random", cfg, createRandomAI);
}

RandomAI::RandomAI(const std::vector<ConfigField>& cfg) {
    uint32_t seed = std::random_device{}();
    if (getConfigValue<bool>(cfg, "useseed")) {
        seed = static_cast<uint32_t>(getConfigValue<int>(cfg, "rngseed"));
    }
    m_rng.seed(seed);
}

void RandomAI::_set_board(const FEN& fen) {
    m_board.set_from_fen(fen);
}

void RandomAI::_apply_move(const UCI& uci_move) {
    Move move = m_board.move_from_uci(uci_move);
    auto legal_moves = m_board.generate_legal_moves();
    if (std::find(legal_moves.begin(), legal_moves.end(), move) == legal_moves.end()) {
        throw std::invalid_argument("RandomAI::apply_move() - illegal move!");
    }
    m_board.make_move(move);
}

void RandomAI::_undo_move() {
    if (!m_board.undo_move()) {
        throw std::invalid_argument("RandomAI::undo_move() - no previous move!");
    }
}

UCI RandomAI::_compute_move() {
    auto moves = m_board.generate_legal_moves();
    if (moves.empty()) {
        throw std::runtime_error("RandomAI::compute_move() - no legal moves!");
    }

    std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
    return MoveEncoding::to_uci(moves[dist(m_rng)]);
}

#include "ai/ai_minimax.hpp"

#include <limits>

void registerMinimaxAI() {
    auto createMinimaxAI = [](const std::vector<ConfigField>& cfg) {
        return std::make_unique<MinimaxAI>();
    };

    AIRegistry::registerAI("Minimax", {}, createMinimaxAI);
}

void MinimaxAI::set_board(const FEN& fen) {
    m_position.set_board(fen);
}

void MinimaxAI::apply_move(const UCI& uci_move) {
    Move move = m_position.move_from_uci(uci_move);
    auto legal_moves = m_position.generate_legal_moves();
    if(std::find(legal_moves.begin(), legal_moves.end(), move) == legal_moves.end()){
        throw std::invalid_argument("MinimaxAI::apply_move() - illegal move!");
    }
    m_position.make_move(move);
}

void MinimaxAI::undo_move() {
    if(!m_position.undo_move()){
        throw std::invalid_argument("MinimaxAI::undo_move() - no previous move!");
    }
}

UCI MinimaxAI::compute_move() {
    auto pseudo_moves = m_position.generate_pseudo_legal_moves();
    if(pseudo_moves.size() == 0){
        throw std::invalid_argument("MinimaxAI::compute_move() - no legal moves!");
    }

    PlayerColor side = m_position.get_side_to_move();
    int alpha = -std::numeric_limits<int>::max();
    int beta = std::numeric_limits<int>::max();
    Move best_move;

    for (const Move& move : pseudo_moves) {
        m_position.make_move(move);
        if (!m_position.in_check(side)) {
            int score = -_alphaBeta(-beta, -alpha, m_search_depth);
            if (score > alpha) {
                alpha = score;
                best_move = move;
            }
        }
        m_position.undo_move();
    }

    return MoveEncoding::to_uci(best_move);
}


int MinimaxAI::_alphaBeta(int alpha, int beta, int depth_left) {
    if(depth_left == 0) return m_position.get_eval();

    PlayerColor side = m_position.get_side_to_move();
    for(const Move& move : m_position.generate_pseudo_legal_moves()){
        m_position.make_move(move);
        if(!m_position.in_check(side)){
            int score = -_alphaBeta(-beta, -alpha, depth_left - 1);
            if(score >= beta){
                m_position.undo_move();
                return beta; // refutation move found, fail-high node
            }
            if(score > alpha) {
                alpha = score; // best move found so far
            }
        }
        m_position.undo_move();
    }

    return alpha;
}

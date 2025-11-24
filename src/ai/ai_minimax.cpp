#include "ai/ai_minimax.hpp"

#include <limits>

void registerMinimaxAI() {
    auto createMinimaxAI = [](const std::vector<ConfigField>& cfg) {
        return std::make_unique<MinimaxAI>();
    };

    AIRegistry::registerAI("Minimax", {}, createMinimaxAI);
}

void MinimaxAI::_set_board(const FEN& fen) {
    m_position.set_board(fen);
}

void MinimaxAI::_apply_move(const UCI& uci_move) {
    Move move = m_position.get_board().move_from_uci(uci_move);
    auto legal_moves = m_position.get_board().generate_legal_moves();
    if (std::find(legal_moves.begin(), legal_moves.end(), move) == legal_moves.end()) {
        throw std::invalid_argument("MinimaxAI::apply_move() - illegal move!");
    }
    m_position.make_move(move);
}

void MinimaxAI::_undo_move() {
    if (!m_position.undo_move()) {
        throw std::invalid_argument("MinimaxAI::undo_move() - no previous move!");
    }
}

UCI MinimaxAI::_compute_move() {
    PlayerColor side = m_position.get_board().get_side_to_move();
    double alpha = -std::numeric_limits<double>::max();
    double beta = std::numeric_limits<double>::max();
    Move best_move;

    int legal_move_count = 0;
    for (const Move& move : m_position.get_ordered_pseudo_legal_moves()) {
        m_position.make_move(move);
        if (!m_position.get_board().in_check(side)) {
            legal_move_count++;
            double score = -_alpha_beta(-beta, -alpha, m_search_depth);
            if (score > alpha) {
                alpha = score;
                best_move = move;
            }
        }
        m_position.undo_move();
    }

    if (legal_move_count == 0) {
        throw std::invalid_argument("MinimaxAI::compute_move() - no legal moves!");
    }
    return MoveEncoding::to_uci(best_move);
}

double MinimaxAI::_alpha_beta(double alpha, double beta, int depth_left) {
    if (depth_left == 0) return m_position.get_eval();

    PlayerColor side = m_position.get_board().get_side_to_move();
    int legal_move_count = 0;

    for (const Move& move : m_position.get_ordered_pseudo_legal_moves()) {
        m_position.make_move(move);
        if (!m_position.get_board().in_check(side)) {
            legal_move_count++;
            int score = -_alpha_beta(-beta, -alpha, depth_left - 1);
            if (score >= beta) {
                m_position.undo_move();
                return beta; // refutation move found, fail-high node
            }
            if (score > alpha) {
                alpha = score; // best move found so far
            }
        }
        m_position.undo_move();
    }

    if (legal_move_count == 0) {
        if (m_position.get_board().in_check(side)) {
            // checkmate, depth bonus to prefer faster mates
            return -1e9 + (m_search_depth - depth_left);
        } else {
            return 0; // Stalemate
        }
    }

    return alpha;
}

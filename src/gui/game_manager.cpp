#include "gui/game_manager.hpp"
#include "ai/registry.hpp"

GameManager::GameManager() : m_game() {
    m_black_ai = AIRegistry::create("Minimax");  // configurable ai later
    m_black_ai.value()->set_board(m_game.get_board_as_fen());
}

GameManager::~GameManager() = default;

void GameManager::update() {
    _handle_ai_moves();
}

void GameManager::new_game(const FEN& fen) {
    m_game.new_board();
    if(m_black_ai.has_value()){
        m_black_ai.value()->set_board(m_game.get_board_as_fen());
    }
    if(m_white_ai.has_value()){
        m_white_ai.value()->set_board(m_game.get_board_as_fen());
    }
}

bool GameManager::is_human_turn() const {
    PlayerColor turn = m_game.get_side_to_move();
    return !(turn == PlayerColor::White ? m_white_ai : m_black_ai).has_value();
}

bool GameManager::try_play_human_move(const UCI& uci) {
    PlayerColor turn = m_game.get_side_to_move();
    if(!is_human_turn()) return false;
    return _try_make_move(uci);
}

bool GameManager::try_undo_move() {
    bool success = m_game.undo_move();
    if(!success) return false;

    m_ai_move.reset();
    if(m_white_ai.has_value()) {
        m_white_ai_unapplied_moves.push_back("UNDO");
    }
    if(m_black_ai.has_value()) {
        m_black_ai_unapplied_moves.push_back("UNDO");
    }
    return true;
}


void GameManager::_handle_ai_moves() {
    if(is_human_turn()) return;
    auto& ai = (m_game.get_side_to_move() == PlayerColor::White ? m_white_ai : m_black_ai).value();

    if(!ai->is_computing() && !m_ai_move.has_value()) {
        // apply stored moves first to sync state
        auto& moves = m_game.get_side_to_move() == PlayerColor::White ? m_white_ai_unapplied_moves : m_black_ai_unapplied_moves;
        for(const UCI& move : moves) {
            if(move == "UNDO") ai->undo_move();
            else ai->apply_move(move);
        }
        moves.clear();

        // Start move calculation
        m_ai_move = ai->compute_move_async();
    }
    else if(m_ai_move.has_value() && m_ai_move.value()->done) {
        // Move computation finished
        if (m_ai_move.value()->error){
            std::rethrow_exception(m_ai_move.value()->error);
        }
        if(!_try_make_move(m_ai_move.value()->result)){
            throw std::runtime_error(std::string("GameManager::_handle_ai_moves() - AI gave illegal move '") + m_ai_move.value()->result + "'!");
        }
        m_ai_move.reset();
    }
}

bool GameManager::_try_make_move(const UCI& move) {
    bool success = m_game.play_move(move);
    if(!success) return false;

    if(m_white_ai.has_value()) {
        m_white_ai_unapplied_moves.push_back(move);
    }
    if(m_black_ai.has_value()) {
        m_black_ai_unapplied_moves.push_back(move);
    }
    return true;
}

const Chess& GameManager::get_game() const {
    return m_game;
}

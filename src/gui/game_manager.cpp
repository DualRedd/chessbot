#include "gui/game_manager.hpp"
#include "ai/registry.hpp"

// Helper: check if state is terminal
static inline bool _is_terminal_state(Chess::GameState s) {
    return !(s == Chess::GameState::NoCheck || s == Chess::GameState::Check);
}

GameManager::GameManager() : m_game(), m_white_config(), m_black_config() {
    m_white_config.is_human = true;
    m_black_config.is_human = true;
}

GameManager::~GameManager() = default;

void GameManager::on_game_end(std::function<void(Chess::GameState)> cb) { 
    m_on_game_end = std::move(cb); 
}

bool GameManager::game_ended() const {
    return m_game_ended;
}

void GameManager::update() {
    _handle_ai_moves();
}

void GameManager::new_game(const PlayerConfiguration& white_cfg, const PlayerConfiguration& black_cfg, const FEN& fen) {
    m_ai_move.reset();
    m_white_ai_actions.clear();
    m_black_ai_actions.clear();
    if(m_white_ai) m_white_ai->request_stop();
    if(m_black_ai) m_black_ai->request_stop();

    m_white_config = white_cfg;
    m_black_config = black_cfg;
    if(!m_white_config.is_human) {
        m_white_ai_actions.push_back(std::make_pair(AiAction::NewGame, fen));
    }
    if(!m_black_config.is_human) {
        m_black_ai_actions.push_back(std::make_pair(AiAction::NewGame, fen));
    }
    m_game.new_board(fen);

    // reset end state and notify if starting pos is already terminal
    m_game_ended = false;
    GameState state = m_game.get_game_state();
    if(_is_terminal_state(state)) {
        if (m_on_game_end) m_on_game_end(state);
        m_game_ended = true;
    }
}

bool GameManager::is_human_turn() const {
    PlayerColor turn = m_game.get_side_to_move();
    return (turn == PlayerColor::White ? m_white_config : m_black_config).is_human;
}

bool GameManager::try_play_human_move(const UCI& uci) {
    if(!is_human_turn()) return false;
    return _try_make_move(uci);
}

bool GameManager::try_undo_move() {
    bool success = m_game.undo_move();
    if(!success) return false;

    m_game_ended = false;
    m_ai_move.reset();
    if(!m_white_config.is_human) {
        if(m_white_ai) m_white_ai->request_stop();
        m_white_ai_actions.push_back(std::make_pair(AiAction::UndoMove, ""));
    }
    if(!m_black_config.is_human) {
        if(m_black_ai) m_black_ai->request_stop();
        m_black_ai_actions.push_back(std::make_pair(AiAction::UndoMove, ""));
    }

    return true;
}

void GameManager::_handle_ai_moves() {
    if(is_human_turn() || m_game_ended) return;
    auto& ai = m_game.get_side_to_move() == PlayerColor::White ? m_white_ai : m_black_ai;

    if((!ai || !ai->is_computing()) && !m_ai_move) {
        // apply stored actions first to sync state
        auto& actions = m_game.get_side_to_move() == PlayerColor::White ? m_white_ai_actions : m_black_ai_actions;
        for(const auto&[action, desc] : actions) {
            switch (action) {
                case AiAction::MakeMove: ai->apply_move(desc); break;
                case AiAction::UndoMove: ai->undo_move(); break;
                case AiAction::NewGame: {
                    auto& player = m_game.get_side_to_move() == PlayerColor::White ? m_white_config : m_black_config;
                    ai = AIRegistry::create(player.ai_name, player.ai_config);
                    ai->set_board(desc);
                    break;
                }
            }
        }
        actions.clear();

        // Start move calculation
        m_ai_move = ai->compute_move_async();
    }
    else if(m_ai_move && m_ai_move->done) {
        // Move computation finished
        if (m_ai_move->error){
            std::rethrow_exception(m_ai_move->error);
        }
        if(!_try_make_move(m_ai_move->result)){
            throw std::runtime_error(std::string("GameManager::_handle_ai_moves() - AI gave illegal move '") + m_ai_move->result + "'!");
        }
        m_ai_move.reset();
    }
}

bool GameManager::_try_make_move(const UCI& move) {
    if(m_game_ended) return false;

    bool success = m_game.play_move(move);
    if(!success) return false;

    if(!m_white_config.is_human) {
        m_white_ai_actions.push_back(std::make_pair(AiAction::MakeMove, move));
    }
    if(!m_black_config.is_human) {
        m_black_ai_actions.push_back(std::make_pair(AiAction::MakeMove, move));
    }

    Chess::GameState state = m_game.get_game_state();
    if(_is_terminal_state(state) && !m_game_ended) {
        if (m_on_game_end) m_on_game_end(state);
        m_game_ended = true;
    }

    return true;
}

const Chess& GameManager::get_game() const {
    return m_game;
}

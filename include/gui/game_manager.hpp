#pragma once

#include <thread>

#include "../core/chess.hpp"
#include "../core/ai_player.hpp"

/**
 * Class handling game state playing human and/or AI moves.
 */
class GameManager : Chess {
public:
    GameManager();
    ~GameManager();

    /**
     * Update manager status. Call consistently to handle internal events.
     */
    void update();

    /**
     * Start a new game.
     * @param fen FEN string describing the starting position
     */
    void new_game(const FEN& fen = CHESS_START_POSITION);

    /**
     * @return True if it is *not* an AI's turn, else false.
     */
    bool is_human_turn() const;

    /**
     * Handle a human move.
     * @param uci UCI move string
     * @return True if the move was applied, else false.
     */
    bool try_play_human_move(const UCI& uci);

     /**
     * Try undoing a move on the board.
     * @return true if succesfull
     */
    bool try_undo_move();

    /**
     * @return The game associated with this manager.
     */
    const Chess& get_game() const;

private:
    /**
     * Handle getting AI moves.
     */
    void _handle_ai_moves();

    /**
     * Try applying a move on the board.
     * @return true if succesfull
     */
    bool _try_make_move(const UCI& move);
    
private:
    Chess m_game;

    // AI
    std::optional<std::unique_ptr<AIPlayer>> m_white_ai;
    std::optional<std::unique_ptr<AIPlayer>> m_black_ai;
    std::optional<std::shared_ptr<AsyncMoveCompute>> m_ai_move;

    /** Represents an action on the board that needs to be communicated to AI players. */
    enum class AiAction { 
        MakeMove = 0,
        UndoMove = 1,
        NewGame = 2
    };
    std::vector<std::pair<AiAction, std::string>> m_white_ai_actions;
    std::vector<std::pair<AiAction, std::string>> m_black_ai_actions;
};


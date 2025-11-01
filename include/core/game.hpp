#pragma once

#include "board.hpp"

enum class PlayerType {
    Human,
    Bot
};

class Game {
public:
    Game();

    void new_game();
    bool make_move(int from_x, int from_y, int to_x, int to_y);
    const Board& get_board() const;

    PlayerColor current_turn() const;
    void switch_turn();
    bool is_game_over() const;

private:
    Board m_board;
    PlayerColor m_turn;
    bool m_gameOver;
};
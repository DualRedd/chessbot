#include "core/game.hpp"

Game::Game() {
    new_game();
}

void Game::new_game() {
    m_board.reset();
    m_turn = PlayerColor::White;
    m_gameOver = false;
}

bool Game::make_move(int from_x, int from_y, int to_x, int to_y) {
    if (m_gameOver){
        return false;
    }

    const Piece& piece = m_board.get_piece(from_x, from_y);
    if (piece.type == PieceType::None){
        return false;
    }
    if (piece.color != m_turn) {
        return false;
    }

    // TODO: move legality check

    bool success = m_board.move_piece(from_x, from_y, to_x, to_y);
    if (success) {
        switch_turn();
        // TODO: check for checkmate/stalemate
    }
    return success;
}

PlayerColor Game::current_turn() const {
    return m_turn;
}

void Game::switch_turn() {
    m_turn = (m_turn == PlayerColor::White ? PlayerColor::Black : PlayerColor::White);
}

bool Game::is_game_over() const {
    return m_gameOver;
}

const Board& Game::get_board() const {
    return m_board;
}

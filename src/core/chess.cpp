#include "core/chess.hpp"

Chess::Chess() {
    new_board();
}

void Chess::new_board(const std::string fen) {
    m_board.setup(fen);
    m_last_move.reset();
}

bool Chess::play_move(const Move& move) {

    // TODO: legality check

    m_board.move_piece(move);
    m_last_move = move;
    return true;
}

std::optional<Move> Chess::get_last_move() const {
    return m_last_move;
}

const Board& Chess::get_board() const {
    return m_board;
}

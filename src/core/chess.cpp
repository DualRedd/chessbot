#include "core/chess.hpp"

Chess::Chess() {
    new_board();
}

void Chess::new_board(const std::string fen) {
    m_board.setup(fen);
}

bool Chess::play_move(const Move& move) {

    // TODO: legality check

    m_board.move_piece(move);
    return true;
}

const Board& Chess::get_board() const {
    return m_board;
}

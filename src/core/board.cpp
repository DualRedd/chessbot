#include "core/board.hpp"

#include <sstream>

Board::Board() {
    reset();
}

void Board::reset() {
    // Clear all
    for (auto& row : m_board)
        for (auto& piece : row)
            piece = Piece{};

    // Pawns
    for (int i = 0; i < 8; ++i) {
        m_board[1][i] = {PieceType::Pawn, PlayerColor::White};
        m_board[6][i] = {PieceType::Pawn, PlayerColor::Black};
    }

    // Other pieces
    const PieceType backRow[] = {
        PieceType::Rook, PieceType::Knight, PieceType::Bishop, 
        PieceType::Queen, PieceType::King, PieceType::Bishop,
        PieceType::Knight, PieceType::Rook
    };
    for (int i = 0; i < 8; ++i) {
        m_board[0][i] = {backRow[i], PlayerColor::White};
        m_board[7][i] = {backRow[i], PlayerColor::Black};
    }
}

bool Board::move_piece(int from_x, int from_y, int to_x, int to_y) {
    if (!is_inside_board(from_x, from_y) || !is_inside_board(to_x, to_y))
        return false;

    Piece piece = m_board[from_y][from_x];
    if (piece.type == PieceType::None)
        return false;

    m_board[to_y][to_x] = piece;
    m_board[from_y][from_x] = {};
    return true;
}

const Piece& Board::get_piece(int x, int y) const {
    return m_board[y][x];
}

void Board::set_piece(int x, int y, const Piece& piece) {
    if (is_inside_board(x, y))
        m_board[y][x] = piece;
}

bool Board::is_inside_board(int x, int y) const {
    return (x >= 0 && x < 8 && y >= 0 && y < 8);
}

std::string Board::to_string() const {
    std::ostringstream oss;
    for (int y = 7; y >= 0; --y) {
        for (int x = 0; x < 8; ++x) {
            const auto& p = m_board[y][x];
            char c = '.';
            if (p.type != PieceType::None) {
                switch (p.type) {
                    case PieceType::Pawn:   c = 'P'; break;
                    case PieceType::Knight: c = 'N'; break;
                    case PieceType::Bishop: c = 'B'; break;
                    case PieceType::Rook:   c = 'R'; break;
                    case PieceType::Queen:  c = 'Q'; break;
                    case PieceType::King:   c = 'K'; break;
                    default: break;
                }
                if (p.color == PlayerColor::Black){
                    c = std::tolower(c);
                }
            }
            oss << c << ' ';
        }
        oss << '\n';
    }
    oss << '\n';
    return oss.str();
}

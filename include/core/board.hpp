#pragma once

#include <array>
#include <string>

enum class PieceType {
    None,
    Pawn, Knight, Bishop, Rook, Queen, King
};

enum class PlayerColor {
    None,
    White,
    Black
};

struct Piece {
    PieceType type = PieceType::None;
    PlayerColor color = PlayerColor::None;
};

class Board {
public:
    Board();

    void reset();
    bool move_piece(int from_x, int from_y, int to_x, int to_y);
    const Piece& get_piece(int x, int y) const;
    void set_piece(int x, int y, const Piece& piece);

    bool is_inside_board(int x, int y) const;

    /**
     * @return a printable representation of the current board state
     */
    std::string to_string() const;

private:
    std::array<std::array<Piece, 8>, 8> m_board;
};
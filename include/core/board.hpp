#pragma once

#include <array>
#include <string>

enum class PieceType {
    None = 0,
    Pawn = 1, 
    Knight = 2, 
    Bishop = 3, 
    Rook = 4, 
    Queen = 5, 
    King = 6
};

enum class PlayerColor {
    None = 0,
    White = 1,
    Black = 2
};

struct Piece {
    PieceType type = PieceType::None;
    PlayerColor color = PlayerColor::None;

    bool operator==(const Piece& other) const {
        return type == other.type && color == other.color;
    }

    struct Hash {
        std::size_t operator()(const Piece& p) const {
            return static_cast<std::size_t>(p.type) * 16 + static_cast<std::size_t>(p.color);
        }
    };
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
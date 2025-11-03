#include "core/board.hpp"

#include <optional>
#include <sstream>

BoardTile::BoardTile() : file(0), rank(0) {}

BoardTile::BoardTile(int file_, int rank_) : file(file_), rank(rank_)  {}

bool BoardTile::operator==(const BoardTile& other) const {
    return file == other.file && rank == other.rank;
}

bool BoardTile::operator!=(const BoardTile& other) const {
    return !(*this == other);
}

bool BoardTile::valid() const {
    return (file >= 0 && file < CHESSBOARD_SIZE && rank >= 0 && rank < CHESSBOARD_SIZE);
}

Move::Move() : from(), to(), promotion(PieceType::None) {}
Move::Move(BoardTile from_, BoardTile to_, PieceType promotion_): from(from_), to(to_), promotion(promotion_) {}

Move::Move(const std::string& uci) {
    if (uci.size() < 4 || uci.size() > 5){
        throw std::invalid_argument("Invalid UCI move: " + uci);
    }
        
    from.file = uci[0] - 'a';
    from.rank = uci[1] - '1';
    to.file   = uci[2] - 'a';
    to.rank   = uci[3] - '1';
    if (uci.size() == 5) {
        switch (uci[4]) {
            case 'q': promotion = PieceType::Queen; break;
            case 'r': promotion = PieceType::Rook;  break;
            case 'b': promotion = PieceType::Bishop; break;
            case 'n': promotion = PieceType::Knight; break;
            default: throw std::invalid_argument("Invalid UCI move: " + uci);
        }
    }
    if(!valid()){
        throw std::invalid_argument("Invalid UCI move: " + uci);
    }
}

bool Move::operator==(const Move& other) const {
    return from == other.from
        && to == other.to
        && promotion == other.promotion;
}

bool Move::valid() const {
    return from.valid() && to.valid() && promotion != PieceType::Pawn && promotion != PieceType::King;
}

std::string Move::toUCI() const {
    if(!valid()){
        throw std::runtime_error("Cannot convert invalid move to UCI string!");
    }
    std::string uci;
    uci += static_cast<char>('a' + from.file);
    uci += static_cast<char>('1' + from.rank);
    uci += static_cast<char>('a' + to.file);
    uci += static_cast<char>('1' + to.rank);
    switch (promotion) {
        case PieceType::Queen:  uci += 'q'; break;
        case PieceType::Rook:   uci += 'r'; break;
        case PieceType::Bishop: uci += 'b'; break;
        case PieceType::Knight: uci += 'n'; break;
        default: break;
    }
    return uci;
}

Board::Board() {}

void Board::setup(const std::string& fen) {
    // Clear the board
    for (auto& row : m_board){
        for (auto& square : row){
            square = Piece{};
        }
    }

    // Defaults
    m_halfmoves = 0;
    m_fullmoves = 1;
    m_side_to_move = PlayerColor::White;
    m_white_king_side_castle_available = true;
    m_white_queen_side_castle_available = true;
    m_black_king_side_castle_available = true;
    m_black_queen_side_castle_available = true;
    m_en_passant_available = false;
    m_en_passant_target_file = -1;
    m_en_passant_target_rank = -1;

    // Read FEN
    std::istringstream iss(fen);
    std::string board_part, side_part, castling_part, ep_part;
    iss >> board_part >> side_part >> castling_part >> ep_part >> m_halfmoves >> m_fullmoves;
    
    // 1. Piece placement
    if (board_part.empty()) {
        throw std::invalid_argument("Invalid FEN: missing board description!");
    }
    int white_king_count = 0;
    int black_king_count = 0;
    int rank = CHESSBOARD_SIZE-1; // FEN starts from last rank
    int file = 0;
    for (char c : board_part) {
        if (c == '/') {
            if (file != CHESSBOARD_SIZE){
                throw std::invalid_argument("Invalid FEN: too few files in rank!");
            }
            rank--;
            file = 0;
            continue;
        }

        if (std::isdigit(c)) {
            file += c - '0';
            if (file > CHESSBOARD_SIZE){
                throw std::invalid_argument("Invalid FEN: too many files in rank!");
            }
            continue;
        }

        if (file >= CHESSBOARD_SIZE){
            throw std::invalid_argument("Invalid FEN: too many files in rank!");
        }

        Piece piece;
        piece.color = std::isupper(c) ? PlayerColor::White : PlayerColor::Black;
        if(c == 'K') white_king_count++;
        if(c == 'k') black_king_count++;

        switch (std::tolower(c)) {
            case 'p': {
                if((piece.color == PlayerColor::White && rank == CHESSBOARD_SIZE-1) || piece.color == PlayerColor::Black && rank == 0){
                    throw std::invalid_argument("Invalid FEN: unpromoted pawn cannot be on the last rank!");
                }
                piece.type = PieceType::Pawn; break;
            }
            case 'n': piece.type = PieceType::Knight; break;
            case 'b': piece.type = PieceType::Bishop; break;
            case 'r': piece.type = PieceType::Rook; break;
            case 'q': piece.type = PieceType::Queen; break;
            case 'k': piece.type = PieceType::King; break;
            default: throw std::invalid_argument(std::string("Invalid FEN: unknown piece character '") + c + "'!");
        }

        m_board[rank][file] = piece;
        file++;
    }
    if (rank != 0 || file != CHESSBOARD_SIZE){
        throw std::invalid_argument("Invalid FEN: incomplete board!");
    }
    if(white_king_count != 1 || black_king_count != 1){
        throw std::invalid_argument("Invalid FEN: board needs to have exactly one white and one black king!");
    }

    // 2. Side to move (optional)
    if (!side_part.empty()) {
        if (side_part == "w") m_side_to_move = PlayerColor::White;
        else if (side_part == "b") m_side_to_move = PlayerColor::Black;
        else throw std::invalid_argument(std::string("Invalid FEN: unknown side to move description '") + side_part + "'!");
    }

    // 3. Castling rights (optional)
    if (!castling_part.empty()) {
        if(castling_part.size() > 4){
            throw std::invalid_argument("Invalid FEN: castling rights description is too long!");
        }
        for(char c : castling_part) {
            switch (c) {
                case 'K': {
                    if(m_board[0][4] != Piece{PieceType::King, PlayerColor::White} || m_board[0][7] != Piece{PieceType::Rook, PlayerColor::White}){
                        throw std::invalid_argument("Invalid FEN: castling rights description does not match the board state!");
                    }
                    m_white_king_side_castle_available = true;
                    break;
                }
                case 'Q': {
                    if(m_board[0][4] != Piece{PieceType::King, PlayerColor::White} || m_board[0][0] != Piece{PieceType::Rook, PlayerColor::White}){
                        throw std::invalid_argument("Invalid FEN: castling rights description does not match the board state!");
                    }
                    m_white_queen_side_castle_available = true;
                    break;
                }
                case 'k': {
                    if(m_board[7][4] != Piece{PieceType::King, PlayerColor::Black} || m_board[7][7] != Piece{PieceType::Rook, PlayerColor::Black}){
                        throw std::invalid_argument("Invalid FEN: castling rights description does not match the board state!");
                    }
                    m_black_king_side_castle_available = true;
                    break;
                }
                case 'q': {
                    if(m_board[7][4] != Piece{PieceType::King, PlayerColor::Black} || m_board[7][0] != Piece{PieceType::Rook, PlayerColor::Black}){
                        throw std::invalid_argument("Invalid FEN: castling rights description does not match the board state!");
                    }
                    m_black_queen_side_castle_available = true;
                    break;
                }
                default: throw std::invalid_argument(std::string("Invalid FEN: castling rights description unknown character '") + c + "'!");
            }
        }
    }

    // 4. En passant target (optional)
    if (!ep_part.empty() && ep_part != "-") {
        if (ep_part.size() == 2 && ep_part[0] >= 'a' && ep_part[0] <= 'h' && (ep_part[1] == '3' || ep_part[1] == '6')) {
            m_en_passant_available = true;
            m_en_passant_target_file = ep_part[0] - 'a';
            m_en_passant_target_rank = ep_part[1] - '1';
            if(m_side_to_move == PlayerColor::White && m_board[m_en_passant_target_rank][m_en_passant_target_file] != Piece{PieceType::Pawn, PlayerColor::Black}){
                throw std::invalid_argument("Invalid FEN: no black pawn below en passant target!");
            }
            if(m_side_to_move == PlayerColor::Black && m_board[m_en_passant_target_rank][m_en_passant_target_file] != Piece{PieceType::Pawn, PlayerColor::White}){
                throw std::invalid_argument("Invalid FEN: no white pawn above en passant target!");
            }
        }
        else{
            throw std::invalid_argument("Invalid FEN: invalid en passant target!");
        }
    }
}

std::string Board::to_fen() const {
    std::ostringstream oss;

    // 1. Piece placement
    for (int rank = CHESSBOARD_SIZE - 1; rank >= 0; --rank) { // FEN starts from last rank
        int empty_count = 0;
        for (int file = 0; file < CHESSBOARD_SIZE; file++) {
            const Piece& piece = m_board[file][rank];
            if (piece.type == PieceType::None) {
                empty_count++;
                continue;
            }
            if (empty_count > 0) {
                oss << empty_count;
                empty_count = 0;
            }

            char symbol;
            switch (piece.type) {
                case PieceType::Pawn:   symbol = 'p'; break;
                case PieceType::Knight: symbol = 'n'; break;
                case PieceType::Bishop: symbol = 'b'; break;
                case PieceType::Rook:   symbol = 'r'; break;
                case PieceType::Queen:  symbol = 'q'; break;
                case PieceType::King:   symbol = 'k'; break;
                default:                symbol = '?'; break;
            }

            if (piece.color == PlayerColor::White){
                symbol = std::toupper(symbol);
            }
            oss << symbol;
        }
        if (empty_count > 0) oss << empty_count;
        if (rank > 0) oss << '/';
    }

    // 2. Side to move
    oss << ' ' << (m_side_to_move == PlayerColor::White ? 'w' : 'b');

    // 3. Castling rights
    std::string castling;
    if (m_white_king_side_castle_available)  castling += 'K';
    if (m_white_queen_side_castle_available) castling += 'Q';
    if (m_black_king_side_castle_available)  castling += 'k';
    if (m_black_queen_side_castle_available) castling += 'q';
    if (castling.empty()) castling = "-";
    oss << ' ' << castling;

    // 4. En passant target
    if (m_en_passant_available) {
        char file_char = 'a' + m_en_passant_target_file;
        char rank_char = '1' + m_en_passant_target_rank;
        oss << ' ' << file_char << rank_char;
    } 
    else {
        oss << " -";
    }

    // 5. Halfmove and fullmove counters
    oss << ' ' << m_halfmoves << ' ' << m_fullmoves;

    return oss.str();
}

bool Board::move_piece(Move move) {
    // The Move class guarantees it is a valid move within the board
    Piece piece = m_board[move.from.rank][move.from.file];
    if (piece.type == PieceType::None) return false;
    m_board[move.to.rank][move.to.file] = piece;
    m_board[move.from.rank][move.from.file] = {};
    m_side_to_move = m_side_to_move == PlayerColor::White ? PlayerColor::Black : PlayerColor::White;
    return true;
}

const Piece& Board::get_piece(BoardTile tile) const {
    if(!tile.valid()){
        throw std::invalid_argument("Trying to get a piece outside board bounds!");
    }
    return m_board[tile.rank][tile.file];
}

std::vector<Move> Board::get_legal_moves(BoardTile from_tile) const {
    std::vector<Move> moves;
    if (!from_tile.valid()) return moves;

    const Piece& piece = m_board[from_tile.rank][from_tile.file];
    if (piece.type == PieceType::None) return moves;

    auto add_move = [&](BoardTile to_tile, PieceType promo = PieceType::None) {
        if(!to_tile.valid()) return;
        const Piece& target = m_board[to_tile.rank][to_tile.file];
        if (target.color == piece.color) return; // can't capture own piece
        moves.emplace_back(from_tile, to_tile, promo);
    };
    auto add_promo_moves = [&](BoardTile to_tile) {
        for (PieceType promo : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}){
            add_move(to_tile, promo);
        }
    };

    switch (piece.type) {
        case PieceType::Pawn: {
            int dir = (piece.color == PlayerColor::White) ? 1 : -1;
            int start_rank = (piece.color == PlayerColor::White) ? 1 : 6;
            int next_rank = from_tile.rank + dir;

            // Forward 1
            if (next_rank >= 0 && next_rank < CHESSBOARD_SIZE && m_board[next_rank][from_tile.file].type == PieceType::None) {
                // Promotion
                if (next_rank == 0 || next_rank == CHESSBOARD_SIZE-1) {
                    add_promo_moves(BoardTile(from_tile.file, next_rank));
                } else {
                    add_move(BoardTile(from_tile.file, next_rank));
                }

                // Forward 2
                if (from_tile.rank == start_rank && m_board[from_tile.rank + 2*dir][from_tile.file].type == PieceType::None) {
                    add_move(BoardTile(from_tile.file, from_tile.rank + 2*dir));
                }

                // Captures
                for (int df : {-1, 1}) {
                    int next_file = from_tile.file + df;
                    if (next_file < 0 || next_file >= CHESSBOARD_SIZE) continue;

                    const Piece& target = m_board[next_rank][next_file];
                    if (target.type != PieceType::None && target.color != piece.color) {
                        if (next_rank == 0 || next_rank == CHESSBOARD_SIZE-1) {
                            add_promo_moves(BoardTile(next_file, next_rank));
                        } else {
                            add_move(BoardTile(next_file, next_rank));
                        }
                    }
                }
            }
            break;
        }

        case PieceType::Knight: {
            std::pair<int,int> offsets[8] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
            for (auto [dx, dy] : offsets){
                add_move(BoardTile(from_tile.file + dx, from_tile.rank + dy));
            }
            break;
        }

        case PieceType::Bishop: {
            std::pair<int,int> directions[4] = {{1,1},{1,-1},{-1,1},{-1,-1}};
            for (auto [dx, dy] : directions) {
                BoardTile to_tile = from_tile;
                for (to_tile.file += dx, to_tile.rank += dy; to_tile.valid(); to_tile.file += dx, to_tile.rank += dy) {
                    add_move(to_tile);
                    if (get_piece(to_tile).type != PieceType::None) break;
                }
            }
            break;
        }

        case PieceType::Rook: {
            std::pair<int,int> directions[4] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (auto [dx, dy] : directions) {
                BoardTile to_tile = from_tile;
                for (to_tile.file += dx, to_tile.rank += dy; to_tile.valid(); to_tile.file += dx, to_tile.rank += dy) {
                    add_move(to_tile);
                    if (get_piece(to_tile).type != PieceType::None) break;
                }
            }
            break;
        }

        case PieceType::Queen: {
            std::pair<int,int> directions[8] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
            for (auto [dx, dy] : directions) {
                BoardTile to_tile = from_tile;
                for (to_tile.file += dx, to_tile.rank += dy; to_tile.valid(); to_tile.file += dx, to_tile.rank += dy) {
                    add_move(to_tile);
                    if (get_piece(to_tile).type != PieceType::None) break;
                }
            }
            break;
        }

        case PieceType::King: {
            std::pair<int,int> offsets[8] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
            for (auto [dx, dy] : offsets) {
                add_move(BoardTile(from_tile.file + dx, from_tile.rank + dy));
            }

            // TODO: castling

            break;
        }

        default:
            break;
    }

    return moves;
}

bool Board::is_legal_move(const Move& move){
    // TODO
    return true;
}

PlayerColor Board::get_side_to_move() const {
    return m_side_to_move;
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

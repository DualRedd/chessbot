#include "core/chess.hpp"

#include <stdexcept>
#include <algorithm>

Chess::Tile::Tile() : file(0), rank(0) {}
Chess::Tile::Tile(int file_, int rank_) : file(file_), rank(rank_) {}

bool Chess::Tile::operator==(const Tile& other) const {
    return file == other.file && rank == other.rank;
}  

bool Chess::Tile::operator!=(const Tile& other) const {
    return !(operator==(other));
}

int Chess::Tile::to_index() const {
    return rank * 8 + file;
}

bool Chess::Tile::valid() const {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

UCI Chess::uci_create(const Tile& from, const Tile& to, PieceType promotion) {
    if(!from.valid()){
        throw std::invalid_argument("Chess::uci_create() - invalid origin tile!");
    }
    if(!to.valid()){
        throw std::invalid_argument("Chess::uci_create() - invalid target tile!");
    }

    std::string uci;
    uci += ('a' + from.file);
    uci += ('1' + from.rank);
    uci += ('a' + to.file);
    uci += ('1' + to.rank);
    
    switch (promotion) {
        case PieceType::Queen:  uci += 'q'; break;
        case PieceType::Rook:   uci += 'r'; break;
        case PieceType::Bishop: uci += 'b'; break;
        case PieceType::Knight: uci += 'n'; break;
        case PieceType::None:               break;
        default: throw std::invalid_argument("Chess::uci_create() - invalid promotion piece!");
    }

    return uci;
}

std::tuple<Chess::Tile,Chess::Tile,PieceType> Chess::uci_parse(const UCI& uci) {
    if(uci.size() < 4 || uci.size() > 5){
        throw std::invalid_argument("Chess::uci_parse() - invalid input UCI!");
    }

    Chess::Tile from(uci[0]-'a', uci[1]-'1');
    if(!from.valid()) {
        throw std::invalid_argument("Chess::uci_parse() - invalid input UCI!");
    }

    Chess::Tile to(uci[2]-'a', uci[3]-'1');
    if(!to.valid()) {
        throw std::invalid_argument("Chess::uci_parse() - invalid input UCI!");
    }

    PieceType promotion = PieceType::None;
    if(uci.size() == 5){
        switch (uci[4]) {
            case 'q': promotion = PieceType::Queen; break;
            case 'r': promotion = PieceType::Rook; break;
            case 'b': promotion = PieceType::Bishop; break;
            case 'n': promotion = PieceType::Knight; break;
            default: throw std::invalid_argument("Chess::uci_parse() - invalid input UCI!");
        }
    }

    return {from, to, promotion};
}


Chess::Chess() {
    m_legal_moves.reserve(218); // Max number of legal moves in any position
    new_board(CHESS_START_POSITION);
}

void Chess::new_board(const FEN& position) {
    m_board.set_from_fen(position);
    _update_legal_moves();
}

PlayerColor Chess::get_side_to_move() const {
    return m_board.get_side_to_move();
}

Piece Chess::get_piece_at(const Tile& tile) const {
    return m_board.get_piece_at(tile.to_index());
}

std::vector<UCI> Chess::get_legal_moves() const {
    return m_legal_moves;
}

bool Chess::is_legal_move(const UCI& move) const {
    return std::find(m_legal_moves.begin(), m_legal_moves.end(), move) != m_legal_moves.end();
}

bool Chess::play_move(const UCI& move) {
    if(!is_legal_move(move)) return false;
    m_board.make_move(m_board.move_from_uci(move));
    _update_legal_moves();
    return true;
}

bool Chess::undo_move() {
    return m_board.undo_move();
}

std::optional<UCI> Chess::get_last_move() const {
    auto move = m_board.get_last_move();
    if(!move.has_value()) return std::nullopt;
    return MoveEncoding::to_uci(move.value());
}

bool Chess::is_check() const {
    return m_board.in_check(get_side_to_move());
}

bool Chess::is_checkmate() const {
    return is_check() && m_legal_moves.empty();
}

bool Chess::is_stalemate() const {
    return !is_check() && m_legal_moves.empty();
}

void Chess::_update_legal_moves() {
    m_legal_moves.clear();
    for(const Move& move : m_board.generate_legal_moves()){
        m_legal_moves.push_back(MoveEncoding::to_uci(move));
    }
}

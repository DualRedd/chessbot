#include "core/chess.hpp"

#include "core/move_generation.hpp"
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
    if (!from.valid()) {
        throw std::invalid_argument("Chess::uci_create() - invalid origin tile!");
    }
    if (!to.valid()) {
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

std::tuple<Chess::Tile, Chess::Tile, PieceType> Chess::uci_parse(const UCI& uci) {
    if (uci.size() < 4 || uci.size() > 5) {
        throw std::invalid_argument("Chess::uci_parse() - invalid input UCI!");
    }

    Chess::Tile from(uci[0] - 'a', uci[1] - '1');
    if (!from.valid()) {
        throw std::invalid_argument("Chess::uci_parse() - invalid input UCI!");
    }

    Chess::Tile to(uci[2] - 'a', uci[3] - '1');
    if (!to.valid()) {
        throw std::invalid_argument("Chess::uci_parse() - invalid input UCI!");
    }

    PieceType promotion = PieceType::None;
    if (uci.size() == 5) {
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
Chess::~Chess() = default;

void Chess::new_board(const FEN& position) {
    m_position.from_fen(position);
    _update_legal_moves();

    m_zobrist_history.clear();
    m_zobrist_counts.clear();
    m_fen_history.clear();
    m_zobrist_history.push_back(m_position.get_zobrist_hash());
    m_zobrist_counts[m_position.get_zobrist_hash()] = 1;
    m_fen_history.push_back(m_position.to_fen());
}

FEN Chess::get_board_as_fen() const {
    return m_position.to_fen();
}

Color Chess::get_side_to_move() const {
    return m_position.get_side_to_move();
}

Piece Chess::get_piece_at(const Tile& tile) const {
    return m_position.get_piece_at(Square(tile.to_index()));
}

std::vector<UCI> Chess::get_legal_moves() const {
    return m_legal_moves;
}

bool Chess::is_legal_move(const UCI& move) const {
    return std::find(m_legal_moves.begin(), m_legal_moves.end(), move) != m_legal_moves.end();
}

bool Chess::play_move(const UCI& move) {
    if (!is_legal_move(move)) return false;

    m_position.make_move(m_position.move_from_uci(move));
    _update_legal_moves();

    uint64_t key = m_position.get_zobrist_hash();
    m_zobrist_history.push_back(key);
    m_zobrist_counts[key] += 1;
    m_fen_history.push_back(m_position.to_fen());

    return true;
}

bool Chess::undo_move() {
    bool success = m_position.undo_move();
    if (success) {
        _update_legal_moves();

        if (m_zobrist_history.empty()) {
            throw std::runtime_error("Chess::undo_move() - Zobrist history underflow!");
        }
        if (m_fen_history.size() != m_zobrist_history.size()) {
            throw std::runtime_error("Chess::undo_move() - FEN history corrupted!");
        }

        uint64_t last_key = m_zobrist_history.back();
        auto it = m_zobrist_counts.find(last_key);
        if (it == m_zobrist_counts.end()) {
            throw std::runtime_error("Chess::undo_move() - Zobrist history corrupted!");
        }
        if (--(it->second) <= 0) m_zobrist_counts.erase(it);
        m_fen_history.pop_back();
        m_zobrist_history.pop_back();
    }
    return success;
}

std::optional<UCI> Chess::get_last_move() const {
    auto move = m_position.get_last_move();
    if (!move.has_value()) return std::nullopt;
    return MoveEncoding::to_uci(move.value());
}

Chess::GameState Chess::get_game_state() const {
    if (m_position.in_check()) {
        if (m_legal_moves.empty()) {
            return GameState::Checkmate;
        } else {
            return GameState::Check;
        }
    } else {
        if (m_legal_moves.empty()) {
            return GameState::Stalemate;
        } else if (m_position.get_halfmove_clock() >= 100) {
            return GameState::DrawByFiftyMoveRule;
        } else if (_is_insufficient_material()) {
            return GameState::DrawByInsufficientMaterial;
        } else if (_is_threefold_repetition()) {
            return GameState::DrawByThreefoldRepetition;
        } else {
            return GameState::NoCheck;
        }
    }
}

bool Chess::_is_insufficient_material() const {
    int white_bishops = 0;
    int black_bishops = 0;
    int white_knights = 0;
    int black_knights = 0;
    int white_other = 0;
    int black_other = 0;

    for (Square square = Square::A1; square < Square::H8; ++square) {
        Piece piece = m_position.get_piece_at(square);
        PieceType type = to_type(piece);
        Color color = to_color(piece);

        if (type == PieceType::None) continue;

        if(color == Color::White){
            switch( type){
                case PieceType::Bishop: white_bishops++; break;
                case PieceType::Knight: white_knights++; break;
                case PieceType::Pawn:
                case PieceType::Rook:
                case PieceType::Queen:
                    white_other++; break;
                default: break;
            }
        } else {
            switch(type){
                case PieceType::Bishop: black_bishops++; break;
                case PieceType::Knight: black_knights++; break;
                case PieceType::Pawn:
                case PieceType::Rook:
                case PieceType::Queen:
                    black_other++; break;
                default: break;
            }
        }
    }

    if (white_other == 0 && black_other == 0) {
        if (white_bishops == 0 && black_bishops == 0) {
            if (white_knights <= 1 && black_knights <= 1) {
                return true;
            }
        } else if (white_knights == 0 && black_knights == 0) {
            if (white_bishops <= 1 && black_bishops <= 1) {
                return true;
            }
        }
    }

    // TODO: Add more cases?

    return false;
}

bool Chess::_is_threefold_repetition() const {
    uint64_t key = m_position.get_zobrist_hash();
    auto it = m_zobrist_counts.find(key);
    if (it == m_zobrist_counts.end() || it->second < 3) {
        return false;
    }

    // Verify positions to be 100% safe against hash collision.
    // Compare FENs without the last two move counters
    auto strip_counters = [](const FEN& fen) -> FEN {
        int spaces = 0;
        for (size_t i = 0; i < fen.size(); ++i) {
            if (fen[i] == ' ') {
                if (++spaces == 4) {
                    return fen.substr(0, i);
                }
            }
        }
        return fen;
    };

    const FEN cur = strip_counters(m_position.to_fen());
    int exact = 0;
    for (const auto& f : m_fen_history) {
        if (strip_counters(f) == cur && ++exact >= 3) {
            return true;
        }
    }
    return false;
}

void Chess::_update_legal_moves() {
    m_legal_moves.clear();
    MoveList move_list;
    move_list.generate<GenerateType::Legal>(m_position);
    for (size_t i = 0; i < move_list.count(); ++i) {
        m_legal_moves.push_back(MoveEncoding::to_uci(move_list[i]));
    }
}

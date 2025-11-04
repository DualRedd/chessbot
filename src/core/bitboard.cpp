#include "core/bitboard.hpp"

#include <cstring>
#include <sstream>
#include <cassert>

// Assert enums match what the implementation expects
// This order allows some optimizations to loop bounds
static_assert(static_cast<int>(PieceType::Pawn)   == 0, "Bitboard: Pawn enum index must be 0");
static_assert(static_cast<int>(PieceType::Knight) == 1, "Bitboard: Knight enum index must be 1");
static_assert(static_cast<int>(PieceType::Bishop) == 2, "Bitboard: Bishop enum index must be 2");
static_assert(static_cast<int>(PieceType::Rook)   == 3, "Bitboard: Rook enum index must be 3");
static_assert(static_cast<int>(PieceType::Queen)  == 4, "Bitboard: Queen enum index must be 4");
static_assert(static_cast<int>(PieceType::King)   == 5, "Bitboard: King enum index must be 5");
static_assert(static_cast<int>(PlayerColor::White) == 0, "Bitboard: White enum index must be 0");
static_assert(static_cast<int>(PlayerColor::Black) == 1, "Bitboard: Black enum index must be 1");

// Bitboard constants
constexpr Board::Bitboard RANK_0 = 0x00000000000000FFULL;
constexpr Board::Bitboard RANK_1 = 0x000000000000FF00ULL;
constexpr Board::Bitboard RANK_6 = 0x00FF000000000000ULL;
constexpr Board::Bitboard RANK_7 = 0xFF00000000000000ULL;
constexpr Board::Bitboard PROMOTION_RANKS = RANK_0 | RANK_7;

// Helper functions
constexpr inline int square_index(int file, int rank)   { return rank * 8 + file; }
constexpr inline int square_index(char file, char rank) { return (rank - '1') * 8 + (file - 'a'); }
constexpr inline int rank_of(int square)                { return square / 8; }
constexpr inline int file_of(int square)                { return square % 8; }
constexpr inline int lsb(Board::Bitboard b)             { return __builtin_ctzll(b); }
constexpr inline void pop_lsb(Board::Bitboard &b)       { b &= b - 1; }


Board::StoredState::StoredState(Move move_, int halfmoves_) 
    : move(move_), halfmoves(halfmoves_) {}

Board::Board(const FEN& fen){
    set_from_fen(fen);
}

Board::Board(const Board& other, bool copy_history) {
    std::memcpy(m_pieces, other.m_pieces, sizeof(m_pieces));
    std::memcpy(m_occupied, other.m_occupied, sizeof(m_occupied));
    m_occupied_all = other.m_occupied_all;
    std::memcpy(m_piece_on_square, other.m_piece_on_square, sizeof(m_piece_on_square));

    m_side_to_move = other.m_side_to_move;
    m_castling_rights = other.m_castling_rights;
    m_en_passant_square = other.m_en_passant_square;
    m_halfmoves = other.m_halfmoves;
    m_fullmoves = other.m_fullmoves;

    if (copy_history) {
        m_state_history = other.m_state_history;
    }
}

void Board::set_from_fen(const FEN& fen) {
    // Clear the board
    m_occupied_all = 0ULL;
    for(int player = 0; player < 2; player++){
        m_occupied[player] = 0ULL;
        for(int type = 0; type < 6; type++){
            m_pieces[player][type] = 0ULL;
        }
    }
    for(int square = 0; square < 64; square++){
        m_piece_on_square[square] = PieceType::None;
    }

    // Clear state history
    m_state_history.clear();
    
    // Defaults
    m_halfmoves = 0;
    m_fullmoves = 1;
    m_side_to_move = PlayerColor::White;
    m_castling_rights = 0b1111; // all rights
    m_en_passant_square = -1; // none

    // Parse FEN
    std::istringstream iss(fen);
    std::string board_part, side_part, castling_part, ep_part;
    iss >> board_part >> side_part >> castling_part >> ep_part >> m_halfmoves >> m_fullmoves;
    if (board_part.empty()) {
        throw std::invalid_argument("Board::set_from_fen() - FEN missing board description!");
    }
    
    // 1. Piece placement
    int white_king_count = 0;
    int black_king_count = 0;

    int rank = 7;  // FEN starts from rank 8
    int file = 0;
    for (char c : board_part) {
        if (c == '/') {
            if (file != 8) {
                throw std::invalid_argument("Board::set_from_fen() - FEN invalid board description!");
            }
            rank--;
            file = 0;
            continue;
        }

        if (std::isdigit(c)) {
            file += c - '0';
            if (file > 8) {
                throw std::invalid_argument("Board::set_from_fen() - FEN invalid board description!");
            }
            continue;
        }

        if (file >= 8) {
            throw std::invalid_argument("Board::set_from_fen() - FEN invalid board description!");
        }

        PlayerColor color = std::isupper(c) ? PlayerColor::White : PlayerColor::Black;
        PieceType type;

        switch (std::tolower(c)) {
            case 'n': type = PieceType::Knight; break;
            case 'b': type = PieceType::Bishop; break;
            case 'r': type = PieceType::Rook; break;
            case 'q': type = PieceType::Queen; break;
            case 'p': type = PieceType::Pawn; break;
            case 'k':
                type = PieceType::King;
                (color == PlayerColor::White ? white_king_count : black_king_count)++;
                break;
            default:
                throw std::invalid_argument(std::string("Board::set_from_fen() - FEN board description unknown character '") + c + "'!");
        }

        int square = square_index(file, rank);
        m_piece_on_square[square] = PieceType(type);
        m_pieces[(int)color][(int)type] |= MASK_SQUARE[square];
        m_occupied[(int)color] |= MASK_SQUARE[square];
        m_occupied_all |= MASK_SQUARE[square];

        file++;
    }

    if (rank != 0 || file != 8) {
        throw std::invalid_argument("Board::set_from_fen() - FEN invalid board description!");
    }
    if (white_king_count != 1 || black_king_count != 1) {
        throw std::invalid_argument("Board::set_from_fen() - FEN invalid board description! Board must have exactly one king per side.");
    }

    // 2. Side to move (optional)
    if (!side_part.empty()) {
        if (side_part == "w") m_side_to_move = PlayerColor::White;
        else if (side_part == "b") m_side_to_move = PlayerColor::Black;
        else throw std::invalid_argument("Board::set_from_fen() - FEN invalid side to move description!");
    }

    // 3. Castling rights (optional)
    if (!castling_part.empty() && castling_part != "-") {
        if(castling_part.size() > 4){
            throw std::invalid_argument("Board::set_from_fen() - FEN invalid castling rights description!");
        }
        for (char c : castling_part) {
            switch (c) {
                case 'K': m_castling_rights |= (1 << 0); break; // white kingside
                case 'Q': m_castling_rights |= (1 << 1); break; // white queenside
                case 'k': m_castling_rights |= (1 << 2); break; // black kingside
                case 'q': m_castling_rights |= (1 << 3); break; // black queenside
                default: throw std::invalid_argument(std::string("Board::set_from_fen() - FEN castling rights description unknown character '") + c + "'!");
            }
        }
    }

    // 4. En passant target (optional)
    if (!ep_part.empty() && ep_part != "-") {
        if (ep_part.size() == 2 && ep_part[0] >= 'a' && ep_part[0] <= 'h' && (ep_part[1] == '3' || ep_part[1] == '6')){
            int file = ep_part[0] - 'a';
            int rank = ep_part[1] - '1';
            m_en_passant_square = square_index(file, rank);
            if(m_side_to_move == PlayerColor::White && (m_pieces[(int)PlayerColor::Black][(int)PieceType::Pawn] | MASK_SQUARE[square_index(file, rank-1)]) == 0ULL) {
                throw std::invalid_argument("Board::set_from_fen() - FEN invalid en passant description! Missing black pawn below en passant target.");
            }
            if(m_side_to_move == PlayerColor::Black && (m_pieces[(int)PlayerColor::White][(int)PieceType::Pawn] | MASK_SQUARE[square_index(file, rank+1)]) == 0ULL) {
                throw std::invalid_argument("Board::set_from_fen() - FEN invalid en passant description! Missing white pawn above en passant target.");
            }
        }
        else {
            throw std::invalid_argument("Board::set_from_fen() - FEN invalid en passant description!");
        }
    }
}

FEN Board::to_fen() const {
    std::ostringstream oss;

    // 1. Piece placement
    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            Piece piece = get_piece_at(square_index(file, rank));

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
                default: throw std::runtime_error("Board::to_fen() - Unknown piece type encountered during conversion!");
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
    if (m_castling_rights & 0b0001) castling += 'K';
    if (m_castling_rights & 0b0010) castling += 'Q';
    if (m_castling_rights & 0b0100) castling += 'k';
    if (m_castling_rights & 0b1000) castling += 'q';
    if (castling.empty()) castling = "-";
    oss << ' ' << castling;

    // 4. En passant target
    if (m_en_passant_square != -1) {
        oss << ' ' << static_cast<char>('a' + file_of(m_en_passant_square));
        oss << static_cast<char>('1' + rank_of(m_en_passant_square));
    }
    else {
        oss << " -";
    }

    // 5. Halfmove and fullmove counters
    oss << ' ' << m_halfmoves << ' ' << m_fullmoves;

    return oss.str();
}

PlayerColor Board::get_side_to_move() const {
    return m_side_to_move; 
}

Piece Board::get_piece_at(const int square) const {
    for (int color = 0; color < 2; color++) {
        for (int type = 0; type < 6; type++) {
            if (m_pieces[color][type] & MASK_SQUARE[square]){
                return Piece{(PieceType)type, (PlayerColor)color};
            }
        }
    }
    return Piece{PieceType::None, PlayerColor::White};
}

std::vector<Move> Board::generate_pseudo_legal_moves() const {
    std::vector<Move> moves;
    moves.reserve(218); // max legal moves in any position

    // Pawns (separate logic for non-capturing moves and promotions)
    Bitboard home_rank = (m_side_to_move == PlayerColor::White) ? RANK_1 : RANK_6;
    Bitboard last_rank = (m_side_to_move == PlayerColor::White) ? RANK_7 : RANK_0;
    int forward_dir = (m_side_to_move == PlayerColor::White) ? 1 : -1;

    // en passant captures
    if(m_en_passant_square != -1) {
        Bitboard ep_attacks = m_pieces[(int)m_side_to_move][(int)PieceType::Pawn] & MASK_PAWN_ATTACKS[1-(int)m_side_to_move][m_en_passant_square];
        while(ep_attacks){
            int from_square = lsb(ep_attacks);
            moves.emplace_back(MoveEncoding::encode(from_square, m_en_passant_square, PieceType::Pawn, PieceType::Pawn, PieceType::None, false, true));
            pop_lsb(ep_attacks);
        }
    }

    // normal moves
    Bitboard pawns = m_pieces[(int)m_side_to_move][(int)PieceType::Pawn] & ~last_rank;
    while(pawns != 0) {
        int from_square = lsb(pawns);

        // single forward
        int to_square = from_square + forward_dir * 8; // is a valid square, because last rank pawns are excluded
        if ((m_occupied_all & MASK_SQUARE[to_square]) == 0) {
            if (MASK_SQUARE[to_square] & last_rank) {
                for (PieceType promo : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}){
                    moves.emplace_back(MoveEncoding::encode(from_square, to_square, PieceType::Pawn, PieceType::None, promo));
                }
            }
            else {
                moves.emplace_back(MoveEncoding::encode(from_square, to_square, PieceType::Pawn));
                // double forward
                if (MASK_SQUARE[from_square] & home_rank) {
                    int to_square_double = from_square + forward_dir * 16;
                    if ((m_occupied_all & MASK_SQUARE[to_square_double]) == 0) {
                        moves.emplace_back(MoveEncoding::encode(from_square, to_square_double, PieceType::Pawn));
                    }
                }
            }
        }

        // captures
        Bitboard attacks = MASK_PAWN_ATTACKS[(int)m_side_to_move][from_square] & m_occupied[1-(int)m_side_to_move];
        while(attacks != 0){
            to_square = lsb(attacks);
            if(MASK_SQUARE[to_square] & last_rank) {
                for (PieceType promo : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}){
                    moves.emplace_back(MoveEncoding::encode(from_square, to_square, PieceType::Pawn, m_piece_on_square[to_square], promo));
                }
            }
            else{
                moves.emplace_back(MoveEncoding::encode(from_square, to_square, PieceType::Pawn, m_piece_on_square[to_square]));
            }
            pop_lsb(attacks);
        }

        pop_lsb(pawns);
    }


    // All other pieces
    for(int type = 1; type < 6; type++){
        Bitboard pieces = m_pieces[(int)m_side_to_move][type];
        while(pieces != 0) {
            int from_square = lsb(pieces);

            Bitboard attacks = _attacks_from(PieceType(type), m_side_to_move, from_square);
            attacks &= ~m_occupied[(int)m_side_to_move]; // remove attacks on ally pieces

            while(attacks != 0){
                int to_square = lsb(attacks);
                moves.emplace_back(MoveEncoding::encode(from_square, to_square, PieceType(type), m_piece_on_square[to_square]));
                pop_lsb(attacks);
            }

            pop_lsb(pieces);
        }
    }

    return moves;
}

std::vector<Move> Board::generate_legal_moves() const {
    std::vector<Move> legal_moves;
    std::vector<Move> pseudo_legal = generate_pseudo_legal_moves();

    Board copy_board(*this, false); // create a copy without copying move history
    for (Move move : pseudo_legal) {
        copy_board.make_move(move);
        if (!copy_board.in_check(m_side_to_move)) {
            legal_moves.push_back(move);
        }
        copy_board.undo_move();
    }

    return legal_moves;
}

Move Board::move_from_uci(const UCI& uci) {
    // Validate format
    if (uci.size() < 4 || uci.size() > 5) {
        throw std::invalid_argument("Board::move_from_uci() - invalid input UCI!");
    }
    auto valid_file = [](char f) { return f >= 'a' && f <= 'h'; };
    auto valid_rank = [](char r) { return r >= '1' && r <= '8'; };
    if (!valid_file(uci[0]) || !valid_rank(uci[1]) || !valid_file(uci[2]) || !valid_rank(uci[3])) {
        throw std::invalid_argument("Board::move_from_uci() - invalid input UCI!");
    }

    int from = square_index(uci[0], uci[1]);
    int to = square_index(uci[2], uci[3]);
    PieceType piece = m_piece_on_square[from];
    PieceType capture = m_piece_on_square[to];
    PieceType promo = PieceType::None;
    bool is_castle = false;
    bool is_ep = false;

    // Promotion
    if (uci.size() == 5) {
        switch (uci[4]) {
            case 'q': promo = PieceType::Queen; break;
            case 'r': promo = PieceType::Rook; break;
            case 'b': promo = PieceType::Bishop; break;
            case 'n': promo = PieceType::Knight; break;
            default: throw std::invalid_argument("Board::move_from_uci() - invalid promotion piece!");
        }
    }

    // Castling
    if (piece == PieceType::King && std::abs(from - to) == 2) {
        is_castle = true;
    }

    // En passant
    if (piece == PieceType::Pawn && to == m_en_passant_square) {
        is_ep = true;
        capture = PieceType::Pawn; // correct capture type
    }

    return MoveEncoding::encode(from, to, piece, capture, promo, is_castle, is_ep);
}

bool Board::is_square_attacked(const int square, const PlayerColor by) const {
    Bitboard target = MASK_SQUARE[square];

    if (MASK_PAWN_ATTACKS[1-(int)by][square] & m_pieces[(int)by][(int)PieceType::Pawn]) {
        return true;
    }

    if (MASK_KNIGHT_ATTACKS[square] & m_pieces[(int)by][(int)PieceType::Knight]) {
        return true;
    }

    if (MASK_KING_ATTACKS[square] & m_pieces[(int)by][(int)PieceType::King]) {
        return true;
    }

    Bitboard bishops = m_pieces[(int)by][(int)PieceType::Bishop] | m_pieces[(int)by][(int)PieceType::Queen];
    if (_attacks_from(PieceType::Bishop, by, square) & bishops) {
        return true;
    }

    Bitboard rooks = m_pieces[(int)by][(int)PieceType::Rook] | m_pieces[(int)by][(int)PieceType::Queen];
    if (_attacks_from(PieceType::Rook, by, square) & rooks) {
        return true;
    }

    return false;
}

bool Board::in_check(const PlayerColor side) const {
    Bitboard king_bb = m_pieces[(int)side][(int)PieceType::King];
    if (!king_bb) throw std::runtime_error("Bitboard broken internal state: missing king piece!");
    int king_square = lsb(king_bb);
    return is_square_attacked(king_square, side == PlayerColor::White ? PlayerColor::Black : PlayerColor::White);
}

void Board::make_move(const Move move) {
    int from = MoveEncoding::from_sq(move);
    int to = MoveEncoding::to_sq(move);
    PieceType piece = MoveEncoding::piece(move);
    PieceType captured = MoveEncoding::capture(move);
    PieceType promo = MoveEncoding::promo(move);

    // Store to state history
    m_state_history.emplace_back(StoredState(move, m_halfmoves));

    int side = (int)m_side_to_move;
    int opp = 1 - side;

    // Remove moved piece from the origin square
    m_pieces[side][(int)piece] &= ~MASK_SQUARE[from];
    m_occupied[side] &= ~MASK_SQUARE[from];
    m_piece_on_square[from] = PieceType::None;

    // Remove captured piece if needed
    if (captured != PieceType::None) {
        m_pieces[opp][(int)captured] &= ~MASK_SQUARE[to];
        m_occupied[opp] &= ~MASK_SQUARE[to];
    }

    // Handle promotion
    if (promo != PieceType::None) {
        m_pieces[side][(int)promo] |= MASK_SQUARE[to]; // Place promoted piece
        m_piece_on_square[to] = promo;
    }
    else {
        m_pieces[side][(int)piece] |= MASK_SQUARE[to]; // Place original piece
        m_piece_on_square[to] = piece;
    }
    m_occupied[side] |= MASK_SQUARE[to];

    // Update all occupancy status
    m_occupied_all = m_occupied[0] | m_occupied[1];

    // Update move counters
    if (piece == PieceType::Pawn || captured != PieceType::None) {
        m_halfmoves = 0; // reset on pawn move or capture
    } else {
        m_halfmoves++;
    }
    if (m_side_to_move == PlayerColor::Black) m_fullmoves++;

    // Next turn
    m_side_to_move = (m_side_to_move == PlayerColor::White) ? PlayerColor::Black: PlayerColor::White;
}

bool Board::undo_move() {
    if(m_state_history.size() == 0) return false;

    // Get previous state and move
    StoredState state = m_state_history.back();
    m_state_history.pop_back();
    Move& move = state.move;

    int from = MoveEncoding::from_sq(move);
    int to = MoveEncoding::to_sq(move);
    PieceType piece = MoveEncoding::piece(move);
    PieceType captured = MoveEncoding::capture(move);
    PieceType promo = MoveEncoding::promo(move);

    // Previous turn
    m_side_to_move = (m_side_to_move == PlayerColor::White) ? PlayerColor::Black: PlayerColor::White;

    int side = (int)m_side_to_move;
    int opp = 1 - side;

    // Handle promotion
    if (promo != PieceType::None) {
        m_pieces[side][(int)promo] &= ~MASK_SQUARE[to]; // Remove promoted piece
    }
    else {
        m_pieces[side][(int)piece] &= ~MASK_SQUARE[to]; // Remove original piece
    }
    m_occupied[side] &= ~MASK_SQUARE[to];
    m_piece_on_square[to] = PieceType::None;

    // Add captured piece if needed
    if (captured != PieceType::None) {
        m_pieces[opp][(int)captured] |= MASK_SQUARE[to];
        m_occupied[opp] |= MASK_SQUARE[to];
        m_piece_on_square[to] = captured;
    }

    // Add moved piece to the origin square
    m_pieces[side][(int)piece] |= MASK_SQUARE[from];
    m_occupied[side] |= MASK_SQUARE[from];
    m_piece_on_square[from] = piece;

    // Update all occupancy status
    m_occupied_all = m_occupied[0] | m_occupied[1];

    // Update move counters
    if (m_side_to_move == PlayerColor::Black) {
        m_fullmoves--;
    }
    m_halfmoves = state.halfmoves;
    return true;
}

std::optional<Move> Board::get_last_move() const {
    if(m_state_history.size() == 0) return std::nullopt;
    else return m_state_history.back().move;
}

Board::Bitboard Board::_attacks_from(const PieceType type, const PlayerColor color, const int square) const {
    Bitboard attacks = 0ULL;
    int file = file_of(square);
    int rank = rank_of(square);

    auto add_ray = [&](int dx, int dy) {
        int tr = rank + dy;
        int tf = file + dx;
        while (tr >= 0 && tr < 8 && tf >= 0 && tf < 8) {
            int square = square_index(tf, tr);
            attacks |= MASK_SQUARE[square];
            if (m_occupied_all & MASK_SQUARE[square]) break;
            tr += dy;
            tf += dx;
        }
    };

    switch (type) {
        case PieceType::Bishop:
            add_ray(1, 1);
            add_ray(1, -1);
            add_ray(-1, 1);
            add_ray(-1, -1);
            break;

        case PieceType::Rook:
            add_ray(1, 0);
            add_ray(-1, 0);
            add_ray(0, 1);
            add_ray(0, -1);
            break;

        case PieceType::Queen:
            add_ray(1, 0);
            add_ray(-1, 0);
            add_ray(0, 1);
            add_ray(0, -1);
            add_ray(1, 1);
            add_ray(1, -1);
            add_ray(-1, 1);
            add_ray(-1, -1);
            break;

        case PieceType::Knight:
            return MASK_KNIGHT_ATTACKS[square];

        case PieceType::King:
            return MASK_KING_ATTACKS[square];

        case PieceType::Pawn:
            return MASK_PAWN_ATTACKS[(int)color][square];

        default:
            return 0ULL;
    }

    return attacks;
}

void Board::_init_masks() {
    for (int square = 0; square < 64; square++){
        MASK_SQUARE[square] = 1ULL << square;
    }

    for (int square = 0; square < 64; square++) {
        int file = file_of(square);
        int rank = rank_of(square);

        // Pawn attacks
        MASK_PAWN_ATTACKS[(int)PlayerColor::White][square] = 0ULL;
        MASK_PAWN_ATTACKS[(int)PlayerColor::Black][square] = 0ULL;
        if (rank < 7) {
            if (file > 0) MASK_PAWN_ATTACKS[(int)PlayerColor::White][square] |= MASK_SQUARE[square_index(file-1, rank+1)];
            if (file < 7) MASK_PAWN_ATTACKS[(int)PlayerColor::White][square] |= MASK_SQUARE[square_index(file+1, rank+1)];
        }
        if (rank > 0) {
            if (file > 0) MASK_PAWN_ATTACKS[(int)PlayerColor::Black][square] |= MASK_SQUARE[square_index(file-1, rank-1)];
            if (file < 7) MASK_PAWN_ATTACKS[(int)PlayerColor::Black][square] |= MASK_SQUARE[square_index(file+1, rank-1)];
        }

        // Knights
        MASK_KNIGHT_ATTACKS[square] = 0ULL;
        const int knight_offsets[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
        for (auto[df, dr] : knight_offsets) {
            if (file+df >= 0 && file+df < 8 && rank+dr >= 0 && rank+dr <= 8){
                MASK_KNIGHT_ATTACKS[square] |= MASK_SQUARE[square_index(file+df, rank+dr)];
            }
        }

        // King
        MASK_KING_ATTACKS[square] = 0ULL;
        for (int dr = -1; dr <= 1; dr++) {
            for (int df = -1; df <= 1; df++) {
                if (file+df >= 0 && file+df < 8 && rank+dr >= 0 && rank+dr <= 8){
                    MASK_KING_ATTACKS[square] |= MASK_SQUARE[square_index(file+df, rank+dr)];
                }
            }
        }

        // Sliding masks
        MASK_BISHOP[square] = 0ULL;
        MASK_ROOK[square] = 0ULL;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int df = -1; df <= 1; ++df) {
                if(dr == 0 && df == 0) continue;
                int tf = file + df, tr = rank + dr;
                while (tf >= 0 && tf < 8 && tr >= 0 && tr < 8) {
                    if (abs(dr) == abs(df)){
                        MASK_BISHOP[square] |= MASK_SQUARE[square_index(tf, tr)];
                    }
                    else {
                        MASK_ROOK[square] |= MASK_SQUARE[square_index(tf, tr)];
                    }
                    tr += dr;
                    tf += df;
                }
            }
        }
    }
}

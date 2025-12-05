#include "core/position.hpp"

#include <cstring>
#include <sstream>

Position::StoredState::StoredState(Move move, Piece captured_piece, uint8_t castling_rights,
            Square en_passant_square, uint32_t halfmoves, uint64_t zobrist, Bitboard pinned) 
  : move(move),
    captured_piece(captured_piece),
    castling_rights(castling_rights),
    en_passant_square(en_passant_square),
    zobrist(zobrist),
    halfmoves(halfmoves),
    pinned(pinned) {}


Position::Position(const FEN& fen) {
    from_fen(fen);
}

Position::Position(const Position& other, bool copy_history) {
    std::memcpy(m_pieces, other.m_pieces, sizeof(m_pieces));
    std::memcpy(m_occupied, other.m_occupied, sizeof(m_occupied));
    m_occupied_all = other.m_occupied_all;
    std::memcpy(m_piece_on_square, other.m_piece_on_square, sizeof(m_piece_on_square));

    m_pinned = other.m_pinned;
    m_pinned_calculated = other.m_pinned_calculated;

    m_side_to_move = other.m_side_to_move;
    m_castling_rights = other.m_castling_rights;
    m_en_passant_square = other.m_en_passant_square;
    m_halfmoves = other.m_halfmoves;
    m_fullmoves = other.m_fullmoves;

    if (copy_history) {
        m_state_history = other.m_state_history;
    }
}

void Position::from_fen(const FEN& fen) {
    // Clear the board
    m_occupied_all = 0ULL;
    for (int color = 0; color < 2; ++color) {
        m_occupied[color] = 0ULL;
        for (int piece = 0; piece < 6; ++piece) {
            m_pieces[color][piece] = 0ULL;
        }
    }
    for (int square = 0; square < 64; square++) {
        m_piece_on_square[square] = Piece::None;
    }

    // Clear state history
    m_state_history.clear();

    // Defaults
    m_halfmoves = 0;
    m_fullmoves = 1;
    m_side_to_move = Color::White;
    m_castling_rights = 0;
    m_en_passant_square = Square::None;
    m_zobrist = 0ULL;
    m_pinned = 0ULL;
    m_pinned_calculated = false;

    // Parse FEN
    std::istringstream iss(fen);
    std::string board_part, side_part, castling_part, ep_part, token;
    iss >> board_part >> side_part >> castling_part >> ep_part;
    if(iss >> token){
        try { m_halfmoves = std::stoul(token); }
        catch(...) { throw std::invalid_argument("Position::from_fen() - FEN invalid halfmove count!"); }
    }
    if(iss >> token){
        try {  m_fullmoves = std::stoul(token); }
        catch(...) { throw std::invalid_argument("Position::from_fen() - FEN invalid fullmove count!"); }
    }

    if (board_part.empty()) {
        throw std::invalid_argument("Position::from_fen() - FEN missing board description!");
    }

    // 1. Piece placement
    int white_king_count = 0;
    int black_king_count = 0;

    int rank = 7; // FEN starts from rank 8
    int file = 0;
    for (char c : board_part) {
        if (c == '/') {
            if (file != 8) {
                throw std::invalid_argument("Position::from_fen() - FEN invalid board description!");
            }
            rank--;
            file = 0;
            continue;
        }

        if (std::isdigit(c)) {
            file += c - '0';
            if (file > 8) {
                throw std::invalid_argument("Position::from_fen() - FEN invalid board description!");
            }
            continue;
        }

        if (file >= 8) {
            throw std::invalid_argument("Position::from_fen() - FEN invalid board description!");
        }

        Color color = std::isupper(c) ? Color::White : Color::Black;
        PieceType type;

        switch (std::tolower(c)) {
            case 'n': type = PieceType::Knight; break;
            case 'b': type = PieceType::Bishop; break;
            case 'r': type = PieceType::Rook; break;
            case 'q': type = PieceType::Queen; break;
            case 'p': type = PieceType::Pawn; break;
            case 'k':
                type = PieceType::King;
                (color == Color::White ? white_king_count : black_king_count)++;
                break;
            default:
                throw std::invalid_argument(std::string("Position::from_fen() - FEN board description unknown character '") + c + "'!");
        }

        Square square = create_square(file, rank);
        m_piece_on_square[+square] = create_piece(color, type);
        m_pieces[+color][+type] |= MASK_SQUARE[+square];
        m_occupied[+color] |= MASK_SQUARE[+square];
        m_occupied_all |= MASK_SQUARE[+square];

        file++;
    }
    if (rank != 0 || file != 8) {
        throw std::invalid_argument("Position::from_fen() - FEN invalid board description!");
    }

    // 2. Side to move (optional)
    if (!side_part.empty()) {
        if (side_part == "w") m_side_to_move = Color::White;
        else if (side_part == "b") m_side_to_move = Color::Black;
        else throw std::invalid_argument("Position::from_fen() - FEN invalid side to move description!");
    }

    // 3. Castling rights (optional)
    if (!castling_part.empty() && castling_part != "-") {
        if (castling_part.size() > 4) {
            throw std::invalid_argument("Position::from_fen() - FEN invalid castling rights description!");
        }
        for (char c : castling_part) {
            switch (c) {
                case 'K': { // white kingside
                    if(m_piece_on_square[+king_start_square(Color::White)] != Piece::WKing
                    || m_piece_on_square[+king_start_square(Color::White)+3] != Piece::WRook)
                    {
                        throw std::invalid_argument("Position::from_fen() - FEN castling rights description does not match board state! '" + fen + "'");
                    }
                    m_castling_rights |= castling_flag(Color::White, CastlingSide::KingSide);  break; 
                }
                case 'Q': { // white queenside
                    if(m_piece_on_square[+king_start_square(Color::White)] != Piece::WKing
                    || m_piece_on_square[+king_start_square(Color::White)-4] != Piece::WRook)
                    {
                        throw std::invalid_argument("Position::from_fen() - FEN castling rights description does not match board state! '" + fen + "'");
                    }
                    m_castling_rights |= castling_flag(Color::White, CastlingSide::QueenSide); break;
                }
                case 'k': { // black kingside
                    if(m_piece_on_square[+king_start_square(Color::Black)] != Piece::BKing
                    || m_piece_on_square[+king_start_square(Color::Black)+3] != Piece::BRook)
                    {
                        throw std::invalid_argument("Position::from_fen() - FEN castling rights description does not match board state! '" + fen + "'");
                    }
                    m_castling_rights |= castling_flag(Color::Black, CastlingSide::KingSide); break; 
                }
                case 'q': { // black queenside
                    if(m_piece_on_square[+king_start_square(Color::Black)] != Piece::BKing
                    || m_piece_on_square[+king_start_square(Color::Black)-4] != Piece::BRook)
                    {
                        throw std::invalid_argument("Position::from_fen() - FEN castling rights description does not match board state! '" + fen + "'");
                    }
                    m_castling_rights |= castling_flag(Color::Black, CastlingSide::QueenSide); break; 
                }
                default: throw std::invalid_argument(std::string("Position::from_fen() - FEN castling rights description unknown character '") + c + "'!");
            }
        }
    }

    // 4. En passant target (optional)
    if (!ep_part.empty() && ep_part != "-") {
        if (ep_part.size() != 2 || ep_part[0] < 'a' || ep_part[0] > 'h' || ep_part[1] != (m_side_to_move == Color::White ? '6' : '3')) {
            throw std::invalid_argument("Position::from_fen() - FEN invalid en passant description!");
        }

        int file = ep_part[0] - 'a';
        int rank = ep_part[1] - '1';
        m_en_passant_square = create_square(file, rank);

        Square capture_piece_square = m_en_passant_square + (m_side_to_move == Color::White ? Shift::Down : Shift::Up);
        if ((m_pieces[+opponent(m_side_to_move)][+PieceType::Pawn] & MASK_SQUARE[+capture_piece_square]) == 0ULL) {
            throw std::invalid_argument("Position::from_fen() - FEN invalid en passant description! Missing pawn to capture.");
        }
    }

    // Position legality check
    if (white_king_count != 1 || black_king_count != 1) {
        throw std::invalid_argument("Position::from_fen() - illegal FEN! Position must have exactly one king per side.");
    }
    if (in_check(opponent(m_side_to_move))) {
        throw std::invalid_argument("Position::from_fen() - illegal FEN! King capture possible.");
    }
    if (PROMOTION_RANKS & (m_pieces[+Color::White][+PieceType::Pawn] | m_pieces[+Color::Black][+PieceType::Pawn])) {
        throw std::invalid_argument("Position::from_fen() - illegal FEN! Unpromoted pawn on last rank.");
    }

    // Compute Zobrist hash for current position
    for (int color = 0; color < 2; ++color) {
        for (int piece = 0; piece < 6; ++piece) {
            Bitboard bb = m_pieces[color][piece];
            while (bb) {
                Square square = lsb(bb);
                m_zobrist ^= ZOBRIST_PIECE[color][piece][+square];
                pop_lsb(bb);
            }
        }
    }
    m_zobrist ^= ZOBRIST_CASTLING[m_castling_rights & 0x0F];
    if (m_en_passant_square != Square::None) m_zobrist ^= ZOBRIST_EP[+file_of(m_en_passant_square)];
    if (m_side_to_move == Color::Black) m_zobrist ^= ZOBRIST_SIDE;

    // Compute pinned pieces
    _calculate_pinned(m_side_to_move);
}

FEN Position::to_fen() const {
    std::ostringstream oss;

    // 1. Piece placement
    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            Piece piece = get_piece_at(create_square(file, rank));

            if (to_type(piece) == PieceType::None) {
                empty_count++;
                continue;
            }
            if (empty_count > 0) {
                oss << empty_count;
                empty_count = 0;
            }

            char symbol;
            switch (to_type(piece)) {
                case PieceType::Pawn:   symbol = 'p'; break;
                case PieceType::Knight: symbol = 'n'; break;
                case PieceType::Bishop: symbol = 'b'; break;
                case PieceType::Rook:   symbol = 'r'; break;
                case PieceType::Queen:  symbol = 'q'; break;
                case PieceType::King:   symbol = 'k'; break;
                default: throw std::runtime_error("Position::to_fen() - Unknown piece type encountered during conversion!"); // GCOVR_EXCL_LINE: should never be hit
            }

            if (to_color(piece) == Color::White) {
                symbol = std::toupper(symbol);
            }
            oss << symbol;
        }

        if (empty_count > 0) oss << empty_count;
        if (rank > 0) oss << '/';
    }

    // 2. Side to move
    oss << ' ' << (m_side_to_move == Color::White ? 'w' : 'b');

    // 3. Castling rights
    std::string castling;
    if (m_castling_rights & castling_flag(Color::White, CastlingSide::KingSide))  castling += 'K';
    if (m_castling_rights & castling_flag(Color::White, CastlingSide::QueenSide)) castling += 'Q';
    if (m_castling_rights & castling_flag(Color::Black, CastlingSide::KingSide))  castling += 'k';
    if (m_castling_rights & castling_flag(Color::Black, CastlingSide::QueenSide)) castling += 'q';
    if (castling.empty()) castling = "-";
    oss << ' ' << castling;

    // 4. En passant target
    if (m_en_passant_square != Square::None) {
        oss << ' ' << static_cast<char>('a' + (+file_of(m_en_passant_square)));
        oss << static_cast<char>('1' + (+rank_of(m_en_passant_square)));
    } else {
        oss << " -";
    }

    // 5. Halfmove and fullmove counters
    oss << ' ' << m_halfmoves << ' ' << m_fullmoves;

    return oss.str();
}
    
bool Position::in_check(Color side) const {
    assert(m_pieces[+side][+PieceType::King] != 0ULL);
    Square king_sq = lsb(m_pieces[+side][+PieceType::King]);
    return attackers_exist(opponent(side), king_sq, get_pieces());
}

bool Position::in_check() const {
    return in_check(m_side_to_move);
}

void Position::make_move(Move move) {
    const Color opp = opponent(m_side_to_move);
    const Square from = MoveEncoding::from_sq(move);
    const Square to = MoveEncoding::to_sq(move);
    const PieceType promo = MoveEncoding::promo(move);
    const MoveType move_type = MoveEncoding::move_type(move);
    const Piece moved_piece = m_piece_on_square[+from];

    assert(is_valid_square(from) && is_valid_square(to) && from != to);
    assert(m_piece_on_square[+from] != Piece::None);
    assert(to_color(m_piece_on_square[+from]) == m_side_to_move);

    // Determine capture square (handling en passant)
    Square capture_square = to;
    if (move_type == MoveType::EnPassant) {
        capture_square = (m_side_to_move == Color::White) ? (to + Shift::Down) : (to + Shift::Up);

        assert(to == m_en_passant_square);
        assert(m_piece_on_square[+capture_square] == create_piece(opp, PieceType::Pawn));
        assert(m_piece_on_square[+to] == Piece::None);
        assert(rank_of(to) == (m_side_to_move == Color::White ? 5 : 2));
    }

    // Store state to history
    m_state_history.emplace_back(StoredState(move, m_piece_on_square[+capture_square], m_castling_rights,
                                            m_en_passant_square, m_halfmoves, m_zobrist, m_pinned));

    // Update move counters
    m_halfmoves++;
    if (m_side_to_move == Color::Black) m_fullmoves++;

    // Remove moved piece from the origin square
    m_pieces[+m_side_to_move][+to_type(moved_piece)] &= ~MASK_SQUARE[+from];
    m_occupied[+m_side_to_move] &= ~MASK_SQUARE[+from];
    m_piece_on_square[+from] = Piece::None;
    m_zobrist ^= ZOBRIST_PIECE[+m_side_to_move][+to_type(moved_piece)][+from];

    // Remove captured piece
    PieceType captured_piece_type = to_type(m_piece_on_square[+capture_square]);
    if (captured_piece_type != PieceType::None) {
        assert(m_piece_on_square[+capture_square] != Piece::None);
        assert(to_color(m_piece_on_square[+capture_square]) == opp);

        m_pieces[+opp][+captured_piece_type] &= ~MASK_SQUARE[+capture_square];
        m_occupied[+opp] &= ~MASK_SQUARE[+capture_square];
        m_piece_on_square[+capture_square] = Piece::None;
        m_zobrist ^= ZOBRIST_PIECE[+opp][+captured_piece_type][+capture_square];
        m_halfmoves = 0; // reset on capture
    }

    // Place moved piece / promotion on target square
    if (move_type == MoveType::Promotion) {
        m_occupied[+m_side_to_move] |= MASK_SQUARE[+to];
        m_pieces[+m_side_to_move][+promo] |= MASK_SQUARE[+to];
        m_piece_on_square[+to] = create_piece(m_side_to_move, promo);
        m_zobrist ^= ZOBRIST_PIECE[+m_side_to_move][+promo][+to];
    }
    else {
        m_occupied[+m_side_to_move] |= MASK_SQUARE[+to];
        m_pieces[+m_side_to_move][+to_type(moved_piece)] |= MASK_SQUARE[+to];
        m_piece_on_square[+to] = moved_piece;
        m_zobrist ^= ZOBRIST_PIECE[+m_side_to_move][+to_type(moved_piece)][+to];
    }

    // Handle castling
    if (move_type == MoveType::Castle) {
        Square rook_from = to > from ? from + 3 : from - 4;
        Square rook_to = static_cast<Square>((+to + +from) >> 1); // to + from / 2

        assert(from == king_start_square(m_side_to_move));
        assert(m_piece_on_square[+rook_from] == create_piece(m_side_to_move, PieceType::Rook));
        assert(m_piece_on_square[+rook_to] == Piece::None);

        // Add rook
        m_pieces[+m_side_to_move][+PieceType::Rook] |= MASK_SQUARE[+rook_to];
        m_occupied[+m_side_to_move] |= MASK_SQUARE[+rook_to];
        m_piece_on_square[+rook_to] = m_piece_on_square[+rook_from];
        m_zobrist ^= ZOBRIST_PIECE[+m_side_to_move][+PieceType::Rook][+rook_to];

        // Remove rook
        m_pieces[+m_side_to_move][+PieceType::Rook] &= ~MASK_SQUARE[+rook_from];
        m_occupied[+m_side_to_move] &= ~MASK_SQUARE[+rook_from];
        m_piece_on_square[+rook_from] = Piece::None;
        m_zobrist ^= ZOBRIST_PIECE[+m_side_to_move][+PieceType::Rook][+rook_from];
    }

    // Update all occupancy
    m_occupied_all = m_occupied[0] | m_occupied[1];

    // Update castling rights
    if(m_castling_rights && (MASK_CASTLE_FLAG[+from] | MASK_CASTLE_FLAG[+to])) {
        m_zobrist ^= ZOBRIST_CASTLING[m_castling_rights & 0x0F];
        m_castling_rights &= ~(MASK_CASTLE_FLAG[+from] | MASK_CASTLE_FLAG[+to]);
        m_zobrist ^= ZOBRIST_CASTLING[m_castling_rights & 0x0F];
    }

    // Update en passant square
    if (m_en_passant_square != Square::None) {
        m_zobrist ^= ZOBRIST_EP[+file_of(m_en_passant_square)];
    }
    m_en_passant_square = Square::None;
    if (to_type(moved_piece) == PieceType::Pawn) {
        if (std::abs(int(to) - int(from)) == 16) {
            m_en_passant_square = from + ((to > from) ? Shift::Up : Shift::Down);
            m_zobrist ^= ZOBRIST_EP[+file_of(m_en_passant_square)];
        }
        m_halfmoves = 0; // reset on pawn move
    }

    // Next turn
    m_side_to_move = (m_side_to_move == Color::White) ? Color::Black : Color::White;
    m_zobrist ^= ZOBRIST_SIDE;
    m_pinned_calculated = false;
}

bool Position::undo_move() {
    if (m_state_history.size() == 0) return false;

    // Get previous state
    const StoredState& state = m_state_history.back();

    // Previous turn
    m_side_to_move = (m_side_to_move == Color::White) ? Color::Black : Color::White;
    
    const Color opp = opponent(m_side_to_move);
    const Square from = MoveEncoding::from_sq(state.move);
    const Square to = MoveEncoding::to_sq(state.move);
    const PieceType promo = MoveEncoding::promo(state.move);
    const MoveType move_type = MoveEncoding::move_type(state.move);
    const Piece moved_piece = move_type == MoveType::Promotion ? create_piece(m_side_to_move, PieceType::Pawn) : m_piece_on_square[+to];

    // Remove moved piece / promoted piece on target square
    m_occupied[+m_side_to_move] &= ~MASK_SQUARE[+to];
    m_piece_on_square[+to] = Piece::None;
    if (move_type == MoveType::Promotion) {
        m_pieces[+m_side_to_move][+promo] &= ~MASK_SQUARE[+to];
    } else {
        m_pieces[+m_side_to_move][+to_type(moved_piece)] &= ~MASK_SQUARE[+to];
    }

    // Add captured piece (and handle en passant)
    if (state.captured_piece != Piece::None) {
        Square capture_square = to;
        if (move_type == MoveType::EnPassant) {
            capture_square = (m_side_to_move == Color::White) ? (to + Shift::Down) : (to + Shift::Up);
        }
        m_pieces[+opp][+to_type(state.captured_piece)] |= MASK_SQUARE[+capture_square];
        m_occupied[+opp] |= MASK_SQUARE[+capture_square];
        m_piece_on_square[+capture_square] = state.captured_piece;
    }

    // Add moved piece to the origin square
    m_pieces[+m_side_to_move][+to_type(moved_piece)] |= MASK_SQUARE[+from];
    m_occupied[+m_side_to_move] |= MASK_SQUARE[+from];
    m_piece_on_square[+from] = moved_piece;

    // Handle castling
    if (move_type == MoveType::Castle) {
        Square rook_from = to > from ? from + 3 : from - 4;
        Square rook_to = to > from ? from + 1 : from - 1;

        // Add rook
        m_pieces[+m_side_to_move][+PieceType::Rook] |= MASK_SQUARE[+rook_from];
        m_occupied[+m_side_to_move] |= MASK_SQUARE[+rook_from];
        m_piece_on_square[+rook_from] = m_piece_on_square[+rook_to];

        // Remove rook
        m_pieces[+m_side_to_move][+PieceType::Rook] &= ~MASK_SQUARE[+rook_to];
        m_occupied[+m_side_to_move] &= ~MASK_SQUARE[+rook_to];
        m_piece_on_square[+rook_to] = Piece::None;
    }

    // Update all occupancy status
    m_occupied_all = m_occupied[0] | m_occupied[1];

    // Update castling status
    m_castling_rights = state.castling_rights;

    // Update en passant square
    m_en_passant_square = state.en_passant_square;

    // Update move counters
    if (m_side_to_move == Color::Black) m_fullmoves--;
    m_halfmoves = state.halfmoves;

    // restore zobrist
    m_zobrist = state.zobrist;

    // restore pinned
    m_pinned = state.pinned;

    m_state_history.pop_back();
    return true;
}

Move Position::move_from_uci(const UCI& uci) const {
    // Validate format
    if (uci.size() < 4 || uci.size() > 5) {
        throw std::invalid_argument("Position::move_from_uci() - invalid input UCI!");
    }
    auto valid_file = [](char f) { return f >= 'a' && f <= 'h'; };
    auto valid_rank = [](char r) { return r >= '1' && r <= '8'; };
    if (!valid_file(uci[0]) || !valid_rank(uci[1]) || !valid_file(uci[2]) || !valid_rank(uci[3])) {
        throw std::invalid_argument("Position::move_from_uci() - invalid input UCI!");
    }

    Square from = create_square(uci[0] - 'a', uci[1] - '1');
    Square to = create_square(uci[2] - 'a', uci[3] - '1');
    Piece piece = m_piece_on_square[+from];

    // Promotion
    if (uci.size() == 5) {
        PieceType promo = PieceType::None;
        switch (uci[4]) {
            case 'q': promo = PieceType::Queen; break;
            case 'r': promo = PieceType::Rook; break;
            case 'b': promo = PieceType::Bishop; break;
            case 'n': promo = PieceType::Knight; break;
            default: throw std::invalid_argument("Position::move_from_uci() - invalid promotion piece!");
        }
        return MoveEncoding::encode<MoveType::Promotion>(from, to, promo);
    }

    // Castling
    if (to_type(piece) == PieceType::King && std::abs((+from) - (+to)) == 2) {
        return MoveEncoding::encode<MoveType::Castle>(from, to);
    }

    // En passant
    if (to_type(piece) == PieceType::Pawn && to == m_en_passant_square) {
        return MoveEncoding::encode<MoveType::EnPassant>(from, to);
    }

    return MoveEncoding::encode<MoveType::Normal>(from, to);
}

void Position::_calculate_pinned(Color side) const {
    Color opp = opponent(side);
    Square king_sq = lsb(get_pieces(side, PieceType::King)); 
    
    m_pinned = 0ULL;

    Bitboard possible_pinners = (MASK_ROOK_ATTACKS[+king_sq] & (get_pieces(opp, PieceType::Rook) | get_pieces(opp, PieceType::Queen)))
                              | (MASK_BISHOP_ATTACKS[+king_sq] & (get_pieces(opp, PieceType::Bishop) | get_pieces(opp, PieceType::Queen)));
    Bitboard occupancy = get_pieces() ^ possible_pinners;

    while(possible_pinners) {
        Square pinner_sq = lsb(possible_pinners);
        Bitboard blockers = MASK_BETWEEN[+king_sq][+pinner_sq] & occupancy;

        if (popcount(blockers) == 1 && (blockers & m_occupied[+side])) {
            // correct color blocker    
            m_pinned |= blockers;
        }
        pop_lsb(possible_pinners);
    }
}

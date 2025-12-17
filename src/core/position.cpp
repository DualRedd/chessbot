#include "core/position.hpp"

#include <cstring>
#include <sstream>

Position::StoredState::StoredState(Move move, Piece captured_piece, uint8_t castling_rights,
            Square en_passant_square, uint8_t halfmoves, uint64_t key, uint64_t pawn_key,
            std::array<Bitboard, 2> king_blockers, std::array<Bitboard, 2> pinners,
            std::array<bool, 2> pins_computed) 
  : move(move),
    captured_piece(captured_piece),
    castling_rights(castling_rights),
    en_passant_square(en_passant_square),
    key(key),
    pawn_key(pawn_key),
    halfmoves(halfmoves),
    king_blockers(king_blockers),
    pinners(pinners),
    pins_computed(pins_computed) {}

Position::Position(const FEN& fen) {
    from_fen(fen);
    m_state_history.reserve(500);
    m_null_move_en_passant_history.reserve(50);
}

Position::Position(const Position& other, bool copy_history) {
    std::memcpy(m_pieces_by_type, other.m_pieces_by_type, sizeof(m_pieces_by_type));
    std::memcpy(m_pieces_by_color, other.m_pieces_by_color, sizeof(m_pieces_by_color));
    std::memcpy(m_piece_on_square, other.m_piece_on_square, sizeof(m_piece_on_square));

    m_king_blockers = other.m_king_blockers;
    m_pins_computed = other.m_pins_computed;
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
    std::memset(m_pieces_by_type, 0, sizeof(m_pieces_by_type));
    std::memset(m_pieces_by_color, 0, sizeof(m_pieces_by_color));
    std::memset(m_piece_on_square, static_cast<int>(Piece::None), sizeof(m_piece_on_square));

    // Clear state history
    m_state_history.clear();

    // Defaults
    m_halfmoves = 0;
    m_fullmoves = 1;
    m_side_to_move = Color::White;
    m_castling_rights = 0;
    m_en_passant_square = Square::None;
    m_pins_computed.fill(false);
    m_check_squares_computed = false;

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
        m_pieces_by_color[+color] |= MASK_SQUARE[+square];
        m_pieces_by_type[+type] |= MASK_SQUARE[+square];
        m_pieces_by_type[+PieceType::All] |= MASK_SQUARE[+square];

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
                    m_castling_rights |= +CastlingFlag::WhiteKingSide; break; 
                }
                case 'Q': { // white queenside
                    if(m_piece_on_square[+king_start_square(Color::White)] != Piece::WKing
                    || m_piece_on_square[+king_start_square(Color::White)-4] != Piece::WRook)
                    {
                        throw std::invalid_argument("Position::from_fen() - FEN castling rights description does not match board state! '" + fen + "'");
                    }
                    m_castling_rights |= +CastlingFlag::WhiteQueenSide; break;
                }
                case 'k': { // black kingside
                    if(m_piece_on_square[+king_start_square(Color::Black)] != Piece::BKing
                    || m_piece_on_square[+king_start_square(Color::Black)+3] != Piece::BRook)
                    {
                        throw std::invalid_argument("Position::from_fen() - FEN castling rights description does not match board state! '" + fen + "'");
                    }
                    m_castling_rights |= +CastlingFlag::BlackKingSide; break; 
                }
                case 'q': { // black queenside
                    if(m_piece_on_square[+king_start_square(Color::Black)] != Piece::BKing
                    || m_piece_on_square[+king_start_square(Color::Black)-4] != Piece::BRook)
                    {
                        throw std::invalid_argument("Position::from_fen() - FEN castling rights description does not match board state! '" + fen + "'");
                    }
                    m_castling_rights |= +CastlingFlag::BlackQueenSide; break; 
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
        if (get_piece_at(capture_piece_square) != create_piece(opponent(m_side_to_move), PieceType::Pawn)) {
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
    if ((MASK_RANK[0] | MASK_RANK[7]) & get_pieces(PieceType::Pawn)) {
        throw std::invalid_argument("Position::from_fen() - illegal FEN! Unpromoted pawn on last rank.");
    }

    // Compute Zobrist hashes for current position
    m_key = 0ULL;
    m_pawn_key = ZOBRIST_SIDE; // ensures that the key is never zero, but side is not part of the hash
    for(int sq = 0; sq < 64; sq++) {
        Piece piece = get_piece_at(Square(sq));
        assert(piece != Piece::All);
        if (piece != Piece::None) {
            m_key ^= ZOBRIST_PIECE[+piece][sq];
            if (to_type(piece) == PieceType::Pawn)
                m_pawn_key ^= ZOBRIST_PIECE[+piece][sq];
        }
    }
    m_key ^= ZOBRIST_CASTLING[m_castling_rights & 0x0F];
    if (m_en_passant_square != Square::None) m_key ^= ZOBRIST_EP[+file_of(m_en_passant_square)];
    if (m_side_to_move == Color::Black) m_key ^= ZOBRIST_SIDE;
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
    oss << ' ' << (int)m_halfmoves << ' ' << (int)m_fullmoves;

    return oss.str();
}

bool Position::gives_check(Move move) const {
    if (!m_check_squares_computed) {
        _compute_check_squares();
        m_check_squares_computed = true;
    }

    const Color opp = opponent(m_side_to_move);
    const Square from = MoveEncoding::from_sq(move);
    const Square to = MoveEncoding::to_sq(move);
    const Piece moved_piece = get_piece_at(from);

    // direct check
    if (m_check_squares[+to_type(moved_piece)] & MASK_SQUARE[+to])
        return true;

    const Bitboard king_bb = get_pieces(opp, PieceType::King);

    // discovered check
    if (get_king_blockers(opp) & MASK_SQUARE[+from]) {
        // Is check if did not move along the pin line
        return !(MASK_LINE[+from][+to] & king_bb);
    }

    const Square king_sq = lsb(king_bb);
    const MoveType move_type = MoveEncoding::move_type(move);

    if (move_type == MoveType::EnPassant) {
        // Rare case: simulate occupancy after capture for discovered check
        // Other discovered checks are handled above, so only possible one is trough both pawns
        Square capture_square = to - pawn_dir(m_side_to_move);
        Bitboard occ = get_pieces() ^ MASK_SQUARE[+from] ^ MASK_SQUARE[+capture_square] | MASK_SQUARE[+to];
        if (attacks_from<PieceType::Rook>(king_sq, occ) & (get_pieces(m_side_to_move, PieceType::Rook) | get_pieces(m_side_to_move, PieceType::Queen)))
            return true;
        if (attacks_from<PieceType::Bishop>(king_sq, occ) & (get_pieces(m_side_to_move, PieceType::Bishop) | get_pieces(m_side_to_move, PieceType::Queen)))
            return true;
        return false;
    }
    else if (move_type == MoveType::Promotion) {
        const PieceType promo = MoveEncoding::promo(move);
        // Check if promoted piece can attack the king
        switch (promo) {
            case PieceType::Knight:
                return attacks_from<PieceType::Knight>(to, get_pieces() ^ MASK_SQUARE[+from]) & king_bb;
            case PieceType::Bishop:
                return attacks_from<PieceType::Bishop>(to, get_pieces() ^ MASK_SQUARE[+from]) & king_bb;
            case PieceType::Rook:
                return attacks_from<PieceType::Rook>(to, get_pieces() ^ MASK_SQUARE[+from]) & king_bb;
            case PieceType::Queen:
                return attacks_from<PieceType::Queen>(to, get_pieces() ^ MASK_SQUARE[+from]) & king_bb;
        }
    }
    else if (move_type == MoveType::Castle) {
        Square rook_to = static_cast<Square>((+to + +from) >> 1); // to + from / 2
        return m_check_squares[+PieceType::Rook] & MASK_SQUARE[+rook_to];
    }

    // Normal move
    return false;
}

void Position::make_move(Move move) {
    const Color opp = opponent(m_side_to_move);
    const Square from = MoveEncoding::from_sq(move);
    const Square to = MoveEncoding::to_sq(move);
    const PieceType promo = MoveEncoding::promo(move);
    const MoveType move_type = MoveEncoding::move_type(move);
    const Piece moved_piece = get_piece_at(from);

    assert(is_valid_square(from) && is_valid_square(to) && from != to);
    assert(to_color(moved_piece) == m_side_to_move);
    assert(moved_piece != Piece::None && moved_piece != Piece::All);

    // Determine capture (handling en passant)
    const Square capture_square = move_type == MoveType::EnPassant ? to - pawn_dir(m_side_to_move) : to;
    const Piece captured = get_piece_at(capture_square);

    assert(captured != Piece::All);
    assert(move_type != MoveType::EnPassant || to == m_en_passant_square);
    assert(move_type != MoveType::EnPassant || get_piece_at(capture_square) == create_piece(opp, PieceType::Pawn));
    assert(move_type != MoveType::EnPassant || get_piece_at(to) == Piece::None);
    assert(move_type != MoveType::EnPassant || rank_of(to) == (m_side_to_move == Color::White ? 5 : 2));

    // Store state to history
    m_state_history.emplace_back(StoredState(move, captured, m_castling_rights, m_en_passant_square,
                                    m_halfmoves, m_key, m_pawn_key, m_king_blockers, m_pinners, m_pins_computed));

    // Update move counters
    m_halfmoves++;
    m_fullmoves += +m_side_to_move; // Black = 1

    // Remove en passant
    m_key ^= (m_en_passant_square != Square::None) * ZOBRIST_EP[+file_of(m_en_passant_square)];
    m_en_passant_square = Square::None;

    // Remove moved piece from the origin square
    m_pieces_by_type[+to_type(moved_piece)] &= ~MASK_SQUARE[+from];
    m_pieces_by_type[+PieceType::All] &= ~MASK_SQUARE[+from];
    m_pieces_by_color[+m_side_to_move] &= ~MASK_SQUARE[+from];
    m_piece_on_square[+from] = Piece::None;
    m_key ^= ZOBRIST_PIECE[+moved_piece][+from];

    // Remove captured piece
    if (captured != Piece::None) {
        assert(get_piece_at(capture_square) != Piece::None);
        assert(to_color(get_piece_at(capture_square)) == opp);

        if (to_type(captured) == PieceType::Pawn)
            m_pawn_key ^= ZOBRIST_PIECE[+captured][+capture_square];

        m_pieces_by_type[+to_type(captured)] &= ~MASK_SQUARE[+capture_square];
        m_pieces_by_type[+PieceType::All] &= ~MASK_SQUARE[+capture_square];
        m_pieces_by_color[+opp] &= ~MASK_SQUARE[+capture_square];
        m_piece_on_square[+capture_square] = Piece::None;
        m_key ^= ZOBRIST_PIECE[+captured][+capture_square];
        m_halfmoves = 0; // reset on capture
    }

    // Place moved piece / promotion on target square
    // On pawn move add possible en passant square and update pawn hash
    m_pieces_by_type[+PieceType::All] |= MASK_SQUARE[+to];
    m_pieces_by_color[+m_side_to_move] |= MASK_SQUARE[+to];
    if (move_type == MoveType::Promotion) {
        const Piece promo_piece = create_piece(m_side_to_move, promo);
        m_pieces_by_type[+to_type(promo_piece)] |= MASK_SQUARE[+to];
        m_piece_on_square[+to] = promo_piece;
        m_key ^= ZOBRIST_PIECE[+promo_piece][+to];
        m_pawn_key ^= ZOBRIST_PIECE[+moved_piece][+from];
    }
    else {
        m_pieces_by_type[+to_type(moved_piece)] |= MASK_SQUARE[+to];
        m_piece_on_square[+to] = moved_piece;
        m_key ^= ZOBRIST_PIECE[+moved_piece][+to];
        
        if (to_type(moved_piece) == PieceType::Pawn) {
            if (std::abs(int(to) - int(from)) == 16) {
                m_en_passant_square = from + pawn_dir(m_side_to_move);
                m_key ^= ZOBRIST_EP[+file_of(m_en_passant_square)];
            }
            m_pawn_key ^= ZOBRIST_PIECE[+moved_piece][+from] ^ ZOBRIST_PIECE[+moved_piece][+to];
            m_halfmoves = 0; // reset on pawn move
        }
    }

    // Handle castling
    if (move_type == MoveType::Castle) {
        Square rook_from = to > from ? from + 3 : from - 4;
        Square rook_to = static_cast<Square>((+to + +from) >> 1); // to + from / 2

        assert(from == king_start_square(m_side_to_move));
        assert(get_piece_at(rook_from) == create_piece(m_side_to_move, PieceType::Rook));
        assert(get_piece_at(rook_to) == Piece::None);

        m_pieces_by_type[+PieceType::Rook] ^= MASK_SQUARE[+rook_from] | MASK_SQUARE[+rook_to];
        m_pieces_by_type[+PieceType::All] ^= MASK_SQUARE[+rook_from] | MASK_SQUARE[+rook_to];
        m_pieces_by_color[+m_side_to_move] ^= MASK_SQUARE[+rook_from] | MASK_SQUARE[+rook_to];
        m_piece_on_square[+rook_to] = m_piece_on_square[+rook_from];
        m_piece_on_square[+rook_from] = Piece::None;
        m_key ^= ZOBRIST_PIECE[+m_piece_on_square[+rook_to]][+rook_from]
                   ^ ZOBRIST_PIECE[+m_piece_on_square[+rook_to]][+rook_to];
    }

    // Update castling rights
    if(m_castling_rights & (MASK_CASTLE_FLAG[+from] | MASK_CASTLE_FLAG[+to])) {
        m_key ^= ZOBRIST_CASTLING[m_castling_rights & 0x0F];
        m_castling_rights &= ~(MASK_CASTLE_FLAG[+from] | MASK_CASTLE_FLAG[+to]);
        m_key ^= ZOBRIST_CASTLING[m_castling_rights & 0x0F];
    }

    // Next turn
    m_side_to_move = opp;
    m_key ^= ZOBRIST_SIDE;
    m_pins_computed.fill(false);
    m_check_squares_computed = false;
}

bool Position::undo_move() {
    if (m_state_history.size() == 0) return false;

    // Get previous state
    StoredState& state = m_state_history.back();

    // Previous turn
    m_side_to_move = opponent(m_side_to_move);
    
    const Color opp = opponent(m_side_to_move);
    const Square from = MoveEncoding::from_sq(state.move);
    const Square to = MoveEncoding::to_sq(state.move);
    const PieceType promo = MoveEncoding::promo(state.move);
    const MoveType move_type = MoveEncoding::move_type(state.move);
    const Piece moved_piece = move_type == MoveType::Promotion ? create_piece(m_side_to_move, PieceType::Pawn) : m_piece_on_square[+to];
    const Piece& captured = state.captured_piece;

    // Remove moved piece / promoted piece on target square
    m_pieces_by_type[+to_type(m_piece_on_square[+to])] &= ~MASK_SQUARE[+to];
    m_pieces_by_type[+PieceType::All] &= ~MASK_SQUARE[+to];
    m_pieces_by_color[+m_side_to_move] &= ~MASK_SQUARE[+to];
    m_piece_on_square[+to] = Piece::None;

    // Add captured piece (and handle en passant)
    if (captured != Piece::None) {
        const Square capture_square = move_type == MoveType::EnPassant ? to - pawn_dir(m_side_to_move) : to;
        m_pieces_by_type[+to_type(captured)] |= MASK_SQUARE[+capture_square];
        m_pieces_by_type[+PieceType::All] |= MASK_SQUARE[+capture_square];
        m_pieces_by_color[+opp] |= MASK_SQUARE[+capture_square];
        m_piece_on_square[+capture_square] = captured;
    }

    // Add moved piece to the origin square
    m_pieces_by_type[+to_type(moved_piece)] |= MASK_SQUARE[+from];
    m_pieces_by_type[+PieceType::All] |= MASK_SQUARE[+from];
    m_pieces_by_color[+m_side_to_move] |= MASK_SQUARE[+from];
    m_piece_on_square[+from] = moved_piece;

    // Handle castling
    if (move_type == MoveType::Castle) {
        Square rook_from = to > from ? from + 3 : from - 4;
        Square rook_to = static_cast<Square>((+to + +from) >> 1); // to + from / 2

        m_pieces_by_type[+PieceType::Rook] ^= MASK_SQUARE[+rook_from] | MASK_SQUARE[+rook_to];
        m_pieces_by_type[+PieceType::All] ^= MASK_SQUARE[+rook_from] | MASK_SQUARE[+rook_to];
        m_pieces_by_color[+m_side_to_move] ^= MASK_SQUARE[+rook_from] | MASK_SQUARE[+rook_to];
        m_piece_on_square[+rook_from] = m_piece_on_square[+rook_to];
        m_piece_on_square[+rook_to] = Piece::None;
    }

    // Update castling status
    m_castling_rights = state.castling_rights;

    // Update en passant square
    m_en_passant_square = state.en_passant_square;

    // Update move counters
    m_fullmoves -= +m_side_to_move; // Black = 1 
    m_halfmoves = state.halfmoves;

    // restore hashes
    m_key = state.key;
    m_pawn_key = state.pawn_key;

    // restore pinners and blockers state
    std::memcpy(&m_king_blockers, &state.king_blockers, sizeof(m_king_blockers));
    std::memcpy(&m_pinners, &state.pinners, sizeof(m_pinners));
    std::memcpy(&m_pins_computed, &state.pins_computed, sizeof(m_pins_computed));

    // Invalidate check squares
    m_check_squares_computed = false;

    m_state_history.pop_back();
    return true;
}

void Position::make_null_move() {
    // Update move counters
    m_halfmoves++;
    m_fullmoves += +m_side_to_move; // Black = 1

    // Remove en passant
    m_null_move_en_passant_history.push_back(m_en_passant_square);
    m_key ^= (m_en_passant_square != Square::None) * ZOBRIST_EP[+file_of(m_en_passant_square)];
    m_en_passant_square = Square::None;

    // Next turn
    m_side_to_move = opponent(m_side_to_move);
    m_key ^= ZOBRIST_SIDE;

    // Invalidate check squares
    m_check_squares_computed = false;
}

void Position::undo_null_move() {
    // Previous turn
    m_side_to_move = opponent(m_side_to_move);
    m_key ^= ZOBRIST_SIDE;

    // Restore en passant square
    assert(!m_null_move_en_passant_history.empty());
    m_en_passant_square = m_null_move_en_passant_history.back();
    m_null_move_en_passant_history.pop_back();
    m_key ^= (m_en_passant_square != Square::None) * ZOBRIST_EP[+file_of(m_en_passant_square)];

    // Update move counters
    m_fullmoves -= +m_side_to_move; // Black = 1 
    m_halfmoves -= 1;

    // Invalidate check squares
    m_check_squares_computed = false;
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

    int from_file = static_cast<unsigned char>(uci[0]) - static_cast<int>('a');
    int from_rank = static_cast<unsigned char>(uci[1]) - static_cast<int>('1');
    int to_file   = static_cast<unsigned char>(uci[2]) - static_cast<int>('a');
    int to_rank   = static_cast<unsigned char>(uci[3]) - static_cast<int>('1');

    Square from = create_square(from_file, from_rank);
    Square to   = create_square(to_file, to_rank);
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

void Position::_compute_pins(Color side) const {
    m_king_blockers[+side] = m_pinners[+side] = 0ULL;

    const Color opp = opponent(side);
    const Square king_sq = lsb(get_pieces(side, PieceType::King)); 
    
    Bitboard possible_pinners = (MASK_ROOK_ATTACKS[+king_sq] & (get_pieces(opp, PieceType::Rook) | get_pieces(opp, PieceType::Queen)))
                              | (MASK_BISHOP_ATTACKS[+king_sq] & (get_pieces(opp, PieceType::Bishop) | get_pieces(opp, PieceType::Queen)));
    Bitboard occupancy = get_pieces() ^ possible_pinners;
    
    while(possible_pinners) {
        Square pinner_sq = lsb(possible_pinners);
        Bitboard blockers = MASK_BETWEEN[+king_sq][+pinner_sq] & occupancy;

        if (popcount(blockers) == 1) {
            m_king_blockers[+side] |= blockers;
            if (blockers & get_pieces(side)) {
                m_pinners[+side] |= MASK_SQUARE[+pinner_sq];
            }
        }
        pop_lsb(possible_pinners);
    }
}

void Position::_compute_check_squares() const {
    const Color opp = opponent(m_side_to_move);
    const Square king_sq = lsb(get_pieces(opp, PieceType::King));
    m_check_squares[+PieceType::Pawn] = MASK_PAWN_ATTACKS[+opp][+king_sq];
    m_check_squares[+PieceType::Knight] = MASK_KNIGHT_ATTACKS[+king_sq];
    m_check_squares[+PieceType::Rook] = attacks_from<PieceType::Rook>(king_sq, get_pieces());
    m_check_squares[+PieceType::Bishop] = attacks_from<PieceType::Bishop>(king_sq, get_pieces());
    m_check_squares[+PieceType::Queen] = m_check_squares[+PieceType::Rook] | m_check_squares[+PieceType::Bishop];
    m_check_squares[+PieceType::King] = 0; // not possible to check with king
}

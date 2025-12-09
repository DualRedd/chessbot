# Create random opening EPDs from a PGN file, filtering by player Elo.
# Usage: python3 tools/pgn_to_edp_picker.py input.pgn out.epd --samples 200 --min-elo 1800 --max-ply 12 --seed 42
# Depends on python-chess library: pip install python-chess

import argparse
import random
import chess.pgn

ELO_KEYS_WHITE = ("WhiteElo", "WhiteRating", "White_Rating", "Whiteelo", "White_rating")
ELO_KEYS_BLACK = ("BlackElo", "BlackRating", "Black_Rating", "Blackelo", "Black_rating")

def parse_elo(headers, keys):
    for k in keys:
        if k in headers:
            try:
                return int(headers[k])
            except Exception:
                try:
                    return int(float(headers[k]))
                except Exception:
                    pass
    return None

def main():
    p = argparse.ArgumentParser(description="Sample random openings (EPD) from a PGN.")
    p.add_argument("inp", help="Input PGN file")
    p.add_argument("out", help="Output EPD file")
    p.add_argument("--samples", "-n", type=int, default=200, help="Number of openings to produce")
    p.add_argument("--min-elo", type=int, default=0, help="Minimum Elo for both players")
    p.add_argument("--max-ply", type=int, default=12, help="Max ply to sample into the game (>=2 recommended)")
    p.add_argument("--seed", type=int, default=None, help="Random seed for reproducibility")
    p.add_argument("--silent", type=int, default=0, help="Suppress output if set to 1")
    args = p.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    reservoir = []
    total_candidates = 0

    with open(args.inp, "r", encoding="utf-8", errors="replace") as fh:
        while True:
            game = chess.pgn.read_game(fh)
            if game is None:
                break
            headers = game.headers

            welo = parse_elo(headers, ELO_KEYS_WHITE)
            belo = parse_elo(headers, ELO_KEYS_BLACK)
            if welo is None or belo is None:
                # skip games without rating info
                continue
            if welo < args.min_elo or belo < args.min_elo:
                continue

            if not args.silent:
                print(f"Processing game: {headers.get('White','?')} ({welo}) vs {headers.get('Black','?')} ({belo})")

            # collect moves
            moves = list(game.mainline_moves())
            if not moves:
                continue

            max_ply = min(args.max_ply, len(moves))
            if max_ply < 1:
                continue
            # sample ply index in [1..max_ply] (1-based ply count)
            ply = random.randint(1, max_ply)
            board = game.board()
            for mv in moves[:ply]:
                board.push(mv)

            fen = board.fen()
            # EPD lines often end with ';'
            epd_line = fen + ";\n"

            total_candidates += 1
            if len(reservoir) < args.samples:
                reservoir.append(epd_line)
            else:
                break

    # write results (unique & preserve order)
    seen = set()
    out_lines = []
    for ln in reservoir:
        if ln not in seen:
            seen.add(ln)
            out_lines.append(ln)
    with open(args.out, "w", encoding="utf-8") as of:
        of.writelines(out_lines)

    print(f"Collected {len(out_lines)} openings (from {total_candidates} candidate games). Output: {args.out}")

if __name__ == "__main__":
    main()

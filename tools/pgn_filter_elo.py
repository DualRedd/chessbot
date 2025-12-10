#!/usr/bin/env python3
# Usage: python3 tools/pgn_filter_by_elo.py input.pgn output.pgn --min-elo 2400 [--either] [--limit N] [--silent]
# Depends on python-chess library: pip install python-chess

import argparse
import sys
import chess.pgn

ELO_KEYS_WHITE = ("WhiteElo", "WhiteRating", "White_Rating", "Whiteelo", "White_rating")
ELO_KEYS_BLACK = ("BlackElo", "BlackRating", "Black_Rating", "Blackelo", "Black_rating")
TIME_CONTROL_KEYS = ("TimeControl", "Time control", "TimeControl", "TC", "Time")

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

def parse_timecontrol(headers):
    # return (initial_seconds, increment_seconds) or None if not parseable
    for k in TIME_CONTROL_KEYS:
        if k in headers:
            val = headers[k].strip()
            if not val or val in ("?", "-"):
                return None
            # common form: "600+5"
            if '+' in val:
                parts = val.split('+', 1)
                try:
                    initial = int(parts[0])
                except Exception:
                    # some formats use "40/900+0" or similar; try to extract digits before slash
                    p0 = parts[0]
                    if '/' in p0:
                        try:
                            initial = int(p0.split('/', 1)[1])
                        except Exception:
                            return None
                    else:
                        return None
                try:
                    inc = int(parts[1])
                except Exception:
                    inc = 0
                return (initial, inc)
            # sometimes a single number
            try:
                initial = int(val)
                return (initial, 0)
            except Exception:
                # other formats like "40/900" -> time control with moves/time: take seconds after slash
                if '/' in val:
                    try:
                        initial = int(val.split('/', 1)[1])
                        return (initial, 0)
                    except Exception:
                        return None
    return None

def main():
    p = argparse.ArgumentParser(description="Filter PGN by player Elo and time control.")
    p.add_argument("inp", help="Input PGN file")
    p.add_argument("out", help="Output PGN file")
    p.add_argument("--min-elo", type=int, default=0, help="Minimum Elo for both players")
    p.add_argument("--min-white", type=int, help="Minimum Elo for White (overrides --min-elo)")
    p.add_argument("--min-black", type=int, help="Minimum Elo for Black (overrides --min-elo)")
    p.add_argument("--either", action="store_true", help="Accept game if either player meets threshold (default requires both)")
    p.add_argument("--limit", type=int, default=0, help="Stop after writing N games (0 = no limit)")
    p.add_argument("--silent", action="store_true", help="Suppress progress output")
    p.add_argument("--min-initial-time", type=int, default=0,
                   help="Minimum initial time (seconds) to accept (e.g. 300 = 5 minutes).")
    p.add_argument("--min-inc", type=int, default=0,
                   help="Minimum increment (seconds) to require (default 0 = don't require extra increment).")
    p.add_argument("--require-timecontrol", action="store_true",
                   help="Require a parseable TimeControl header; otherwise skip game.")
    args = p.parse_args()

    min_white = args.min_white if args.min_white is not None else args.min_elo
    min_black = args.min_black if args.min_black is not None else args.min_elo
    require_either = args.either

    try:
        fh = open(args.inp, "r", encoding="utf-8", errors="replace")
    except Exception as e:
        print(f"Failed to open input: {e}", file=sys.stderr)
        sys.exit(2)

    out_fh = open(args.out, "w", encoding="utf-8", errors="replace")

    processed = 0
    written = 0
    skipped_no_elo = 0
    skipped_timecontrol = 0
    skipped_no_time_header = 0

    try:
        while True:
            game = chess.pgn.read_game(fh)
            if game is None:
                break
            processed += 1

            headers = game.headers
            welo = parse_elo(headers, ELO_KEYS_WHITE)
            belo = parse_elo(headers, ELO_KEYS_BLACK)

            if welo is None or belo is None:
                skipped_no_elo += 1
                continue

            ok_elo = (welo >= min_white and belo >= min_black) if not require_either else (welo >= min_white or belo >= min_black)
            if not ok_elo:
                continue

            # time control filtering
            tc = parse_timecontrol(headers)
            if tc is None:
                if args.require_timecontrol:
                    skipped_no_time_header += 1
                    continue
                # otherwise accept game (no timecontrol info)
            else:
                initial, inc = tc
                if initial < args.min_initial_time or inc < args.min_inc:
                    skipped_timecontrol += 1
                    continue

            exporter = chess.pgn.StringExporter(headers=True, variations=True, comments=True)
            out_fh.write(game.accept(exporter))
            out_fh.write("\n\n")
            written += 1

            if args.limit and written >= args.limit:
                break

            if not args.silent and (written % 100 == 0):
                print(f"Processed {processed:,}, written {written:,}, skipped_no_elo {skipped_no_elo:,}, skipped_tc {skipped_timecontrol:,}, missing_tc {skipped_no_time_header:,}")

    finally:
        fh.close()
        out_fh.close()

    if not args.silent:
        print("Done.")
        print(f"Processed games: {processed}")
        print(f"Written games : {written}")
        print(f"Skipped (no elo): {skipped_no_elo}")
        print(f"Skipped (timecontrol filter): {skipped_timecontrol}")
        if args.require_timecontrol:
            print(f"Skipped (missing timecontrol): {skipped_no_time_header}")

if __name__ == "__main__":
    main()

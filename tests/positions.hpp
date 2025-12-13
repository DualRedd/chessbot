#pragma once

#include "core/standards.hpp"

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const FEN COMPLEX_POSITION = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

// Varied test positions covering special cases
const FEN TEST_POSITIONS[] {
    CHESS_START_POSITION,
    "rnbqkbnr/pppp1ppp/8/4Q3/3P4/8/PPP1PPPP/RNB1KBNR b KQkq - 0 2",
    "2r4r/p5pp/8/2B1Q2k/1P1P4/2P3P1/P4P1P/4K2R b K - 0 2",
    "2r4r/pB4pp/2b5/3Q4/1P6/2P2kP1/P4P1P/4K2R b K - 0 2",
    "7r/1R1p1kpp/8/4P3/2B5/1P4P1/P6P/3QK3 b - - 0 2",
    "7r/5kpp/8/3pP3/1R2K3/1P4P1/P4B1P/3Q4 w - d6 0 2",
    "b7/8/4k3/3pP3/4K3/8/8/8 w - d6 0 2"

    // single checks
    "8/4k3/3n1p2/6p1/3N1KP1/7P/8/8 w - -  0 1",
    "8/5k2/3n1p2/8/3N2Pp/6K1/8/8 w - -  0 1",
    "8/8/3n2k1/5P2/3N4/6K1/8/8 b - -  0 1",
    "rnb2rk1/p4pbp/3p2p1/qBpP4/4N3/5N1P/PP3PP1/R1BQK2R w KQ -  0 1"
    "r1b2Rk1/p1n3p1/1p4N1/3p4/2pPq3/P3P3/1PQ3PP/R5K1 b - -  0 1",
    "6k1/r2b1p1p/2pq2p1/1p1p4/1P1PN3/1R2Pn1P/2B2PP1/1Q4K1 w - -  0 1",
    "6k1/r4p1p/2p3p1/1p6/1P1PB2q/R3PP2/5P1K/2Q2b2 w - -  0 1",
    "4R1k1/pp5p/3nN1p1/3p4/1P5P/2P2P2/P5P1/6K1 b - -  0 1",

    // double checks
    "3k4/8/8/8/1N1K1q2/8/2B1PN2/3r4 w - - 0 1"
    "4r3/pp4k1/5rnp/2p5/2P1B3/P1N1p3/1P1R1KPP/8 w - -  0 1",
    "5nr1/1pq1k1n1/2p2Prb/2Pp3p/pP1P1P1P/P2Q2P1/1B2R1RK/5N2 b - -  0 1",
    "7r/4k1p1/1q1P2p1/1p4P1/p1p1R3/6P1/PP2Q1K1/8 b - -  0 1",
    "3r4/5B1k/1p5R/5Q2/1PP2q2/3p3K/7P/8 b - -  0 1",
    "8/2k4R/p1b1N3/1p2N1n1/1P1P4/3PK3/8/8 b - -  0 1",
    "r2q1rk1/1pp3pp/p1n1p3/4p3/4P1n1/2P4P/PPBN1KP1/R1BQ3R w - -  0 1"

    // en passants
    "8/8/8/3Pp3/8/8/8/k6K w - e6 0 1",
    "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1",
    "r1b2rk1/1pp1q2p/3p2p1/pP1Pb3/PR1pPp2/3P1P2/2Q1B1PP/2B2RK1 w - a6  0 1",
    "r1bq1rk1/pp1pb1pp/2n1p1n1/4Pp2/2P5/1P1BQN2/PB3PPP/RN3RK1 w - f6  0 1",
    "r1bq1r2/1p1n1pkp/p2p2p1/n1pPp3/2P1P3/2N2P2/PP1Q2PP/R1N1KB1R w KQ c6  0 1",
    "r2q1rk1/pp4pp/1np1p3/4Pp2/3P1P1b/2NQB3/PP4PP/3R1RK1 w - f6  0 1",

    // castling positions
    "rnbqk2r/p3ppPp/5n2/Pp1p4/8/3P1Q2/1pP2PPP/R1B1K2R w KQkq b6 0 1",
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
    "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1",
    "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1",
    "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1",
    "5k2/8/8/8/8/8/8/4K2R w K - 0 1",

    // promotion positions
    "8/P7/8/8/8/8/8/k6K w - - 0 1",
    "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1",
    "4k3/1P6/8/8/8/8/K7/8 w - - 0 1",
    "8/P1k5/K7/8/8/8/8/8 w - - 0 1",

    // Some complex positions
    COMPLEX_POSITION,
    "5BK1/5p1N/5Pp1/6Pk/8/1b6/8/7q b - - 0 1",
    "r1b3kr/pp1n2Bp/2pb2q1/3p3N/3P4/2P2Q2/P1P3PP/4RRK1 w - - 0 1",
    "1r3rk1/3b1p1p/pp1p1p1Q/n1q1p3/2P1P3/P1PB1N2/6PP/1R3RK1 w - - 0 1",
    "2rq2k1/R5pp/8/Q2pnp2/8/1P2P1P1/7P/5B1K w - - 0 1",
    "2k5/1p3pp1/p1p1p1p1/2P1P1P1/PP1P1P1P/K7/8/8 w - - 0 1",
    "2k4N/Q1np4/2p2Bpp/1p1P4/pPP1p2P/P7/7q/1K6 w - - 0 1",
    "r1b1k1r1/1p2np1p/p1n1pQp1/3p4/3NPP2/P2RB3/2PK2PP/q4B1R w q - 0 1",
    "rnb2r2/3pppkp/p5p1/qPpQ4/P1P1n3/4PN2/4KPPP/RN3B1R b - - 0 1",
    "3r1r1k/pp5p/4b1pb/6q1/3P4/4p1BP/PP2Q1PK/3RRB2 b - - 0 1",
};

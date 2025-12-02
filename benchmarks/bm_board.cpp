#include "benchmark/benchmark.h"
#include "core/board.hpp"
#include "ai/search_position.hpp"

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static uint64_t perft(Board &b, int depth) {
    if (depth == 0)
        return 1;

    auto moves = b.generate_legal_moves();

    if (depth == 1)
        return uint64_t(moves.size());

    uint64_t node_count = 0;
    for (auto mv : moves) {
        b.make_move(mv);
        node_count += perft(b, depth - 1);
        b.undo_move();
    }
    return node_count;
}

static void BM_perft(benchmark::State& state) {
    int depth = static_cast<int>(state.range(0));
    Board b(CHESS_START_POSITION);

    for (auto _ : state) {
        uint64_t nodes = perft(b, depth);
        benchmark::DoNotOptimize(nodes);
        state.counters["nodes"] = benchmark::Counter(static_cast<double>(nodes));
        state.counters["nps"] = benchmark::Counter(static_cast<double>(nodes),
                                                   benchmark::Counter::kIsRate);
    }
}

BENCHMARK(BM_perft)->Arg(3)->Arg(4)->Arg(5)->Arg(6)->Unit(benchmark::kMillisecond);
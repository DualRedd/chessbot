#include "benchmark/benchmark.h"
#include "core/position.hpp"
#include "core/move_generation.hpp"

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static uint64_t perft(Position &b, int depth) {
    if (depth == 0)
        return 1;

    MoveList move_list;
    move_list.generate_legal(b);

    uint64_t node_count = 0;
    for (Move& move : move_list) {
        b.make_move(move);
        node_count += perft(b, depth - 1);
        b.undo_move();
    }
    return node_count;
}

static void BM_perft(benchmark::State& state) {
    int depth = static_cast<int>(state.range(0));
    Position b(CHESS_START_POSITION);

    for (auto _ : state) {
        uint64_t nodes = perft(b, depth);
        benchmark::DoNotOptimize(nodes);
        state.counters["nodes"] = benchmark::Counter(static_cast<double>(nodes));
        state.counters["nps"] = benchmark::Counter(static_cast<double>(nodes),
                                                   benchmark::Counter::kIsRate);
    }
}

BENCHMARK(BM_perft)->Arg(3)->Arg(4)->Arg(5)->Arg(6)->Unit(benchmark::kMillisecond);
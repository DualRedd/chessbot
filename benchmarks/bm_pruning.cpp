#include "benchmark/benchmark.h"
#include "engine/minimax_engine.hpp"

static void run_minimax_fixed_depth(benchmark::State& state,
                                    const std::vector<FEN>& positions,
                                    const int depth,
                                    const size_t tt_size_megabytes)
{
    for (auto _ : state) {
        uint64_t total_alpha_beta_nodes = 0;
        uint64_t total_quiescence_nodes = 0;
        uint64_t total_aspiration_miss_nodes = 0;
        uint64_t total_eval = 0; // for verification

        // Fixed-depth search with unlimited time, so that we can compare pruning stats (same game tree)
        for (const FEN& fen : positions) {
            MinimaxAI ai(depth, 1e6, tt_size_megabytes, false);
            ai.set_board(fen);
            ai.compute_move();
            MinimaxAI::Stats s = ai.get_stats();
            total_alpha_beta_nodes += s.alpha_beta_nodes;
            total_quiescence_nodes += s.quiescence_nodes;
            total_aspiration_miss_nodes += s.aspiration_miss_nodes;
            total_eval += s.eval;
        }

        const double n = static_cast<double>(positions.size());
        state.counters["alpha_beta_nodes_avg"] = static_cast<double>(total_alpha_beta_nodes) / n;
        state.counters["quiescence_nodes_avg"] = static_cast<double>(total_quiescence_nodes) / n;
        state.counters["aspiration_miss_nodes_avg"] = static_cast<double>(total_aspiration_miss_nodes) / n;
        state.counters["eval_verification_sum"] = static_cast<double>(total_eval);
    }
}

static const std::vector<FEN> PRUNING_TEST_POSITIONS = {
    "r1bqkbnr/ppp1pppp/2n5/3p4/4P3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 2 3"
    "rnb1kb1r/pp2pppp/2p2n2/q7/3P4/2N2N2/PPP2PPP/R1BQKB1R w KQkq - 0 6",
    "rnb2rk1/2q2ppp/p4n2/1p1Pp3/3N2P1/b3B3/BPP1QP1P/R2NK2R w KQ - 0 14",
    "8/pp1rkn2/2p1p3/2P2pp1/1B6/4bPP1/PPB1P1K1/7R b - - 3 31",
    "r2q1rk1/bp4pp/2p2nn1/p2p4/3P2b1/4BNN1/PP2BPPP/R2QR1K1 w - - 0 19",
    "r4r2/4qppk/2pp3p/b1n1p2P/PR2P1Q1/1BN5/2P2PP1/3R2K1 w - - 2 29",
};

// Benchmark: depth 9, aspiration enabled
static void BM_minimax_pruning_aspiration(benchmark::State& state) {
    int aspiration_window = static_cast<int>(state.range(0));
    run_minimax_fixed_depth(state,
                            PRUNING_TEST_POSITIONS,
                            /*depth=*/10,
                            /*tt_size_megabytes=*/512);
}
BENCHMARK(BM_minimax_pruning_aspiration)
    ->Arg(10)->Arg(20)->Arg(30)->Arg(50)->Arg(100)
    ->Unit(benchmark::kSecond);

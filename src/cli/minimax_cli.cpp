#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>

#include "ai/ai_minimax.hpp"

const FEN CHESS_START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::unique_ptr<MinimaxAI> create_engine() {
    const int depth = 99;
    const double time_limit_seconds = 5.0;
    const size_t tt_size_megabytes = 256ULL;
    const bool aspiration_enabled = true;
    const int aspiration_window = 50;
    const bool enable_info_output = true;

    return std::make_unique<MinimaxAI>(
        depth,
        time_limit_seconds,
        tt_size_megabytes,
        aspiration_enabled,
        aspiration_window,
        enable_info_output
    );
}

int main(int argc, char** argv) {
    const int depth = 99;
    const size_t tt_size_megabytes = 256ULL;
    const bool aspiration_enabled = true;
    const int aspiration_window = 50;

    auto engine = create_engine();
    engine->set_board(CHESS_START_POSITION);

    std::atomic_bool compute_running{false};

    auto start_move_compute = [&](int depth_hint, double movetime_hint_s, int64_t nodes_hint) {
        if (compute_running.load(std::memory_order_acquire)) {
            throw std::runtime_error("Cannot start new move compute: one is already running!");
        }

        compute_running.store(true, std::memory_order_release);

        // inform engine of limits
        engine->set_time_limit_seconds(movetime_hint_s);
        engine->set_max_depth(depth_hint);
        engine->set_max_nodes(nodes_hint);

        std::thread([&]() {
            UCI move = engine->compute_move();
            std::cout << "bestmove " << move << "\n" << std::flush;
            compute_running.store(false, std::memory_order_release);
        }).detach();
    };

    auto stop_compute_and_wait = [&]() {
        if (compute_running.load(std::memory_order_acquire)) {
            engine->request_stop();
            while (compute_running.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    };

    // UCI loop
    std::string line;
    std::cout << std::flush;
    while (std::getline(std::cin, line)) {
        // get next command
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string cmd; iss >> cmd;

        try {
            if (cmd == "uci") {
                std::cout << "id name minimax \n";
                std::cout << "id author Haapiainen\n";
                std::cout << "uciok\n" << std::flush;
            }
            else if (cmd == "isready") {
                std::cout << "readyok\n" << std::flush;
            }
            else if (cmd == "ucinewgame") {
                stop_compute_and_wait();
                engine->clear_transposition_table();
                engine->set_board(CHESS_START_POSITION);
            }
            else if (cmd == "position") {
                stop_compute_and_wait();
                // format: position startpos | fen <fen> [moves ...]
                std::string token; iss >> token;
                if (token == "startpos") {
                    engine->set_board(CHESS_START_POSITION);
                }
                else if (token == "fen") {
                    // read 6 fen fields
                    std::string fen = "";
                    for (int f = 0; f < 6; ++f) {
                        std::string part;
                        if (!(iss >> part)) break;
                        if (f) fen += " ";
                        fen += part;
                    }
                    engine->set_board(fen);
                }
                else {
                    throw std::invalid_argument("Unknown position command format!");
                }
                // optional moves
                std::string moves_token;
                if (iss >> moves_token && moves_token == "moves") {
                    std::string mv;
                    while (iss >> mv) {
                        engine->apply_move(mv);
                    }
                }
            }
            else if (cmd == "go") {
                stop_compute_and_wait();
                // parse go options
                int depth = -1;
                double movetime_s = -1.0;
                int64_t nodes = -1;
                std::string tok;
                while (iss >> tok) {
                    if (tok == "movetime") { iss >> movetime_s; movetime_s /= 1000.0; }
                    else if (tok == "depth") { iss >> depth; }
                    else if (tok == "nodes") { iss >> nodes; }
                    else {
                        throw std::invalid_argument("Unknown go command option: " + tok);
                    }
                }

                start_move_compute(depth, movetime_s, nodes);
            }
            else if (cmd == "stop") {
                stop_compute_and_wait();
            }
            else if (cmd == "setoption") {
                // ignore for now
            }
            else if (cmd == "quit") {
                stop_compute_and_wait();
            }
            else {
                // unrecognized command: ignore
            }
        }
        catch (const std::exception &e) {
            std::cerr << "Runtime error handling command '" << cmd << "': " << e.what() << "\n";
        }
    }

    stop_compute_and_wait();
    return 0;
}
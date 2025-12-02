add_executable(run_benchmarks
    bm_board.cpp
)

target_link_libraries(run_benchmarks PRIVATE
    benchmark::benchmark_main
    chess_core
    chess_bots
    pthread
)

add_executable(run_benchmarks
    bm_perft.cpp
    bm_minimax_ai.cpp
)

target_link_libraries(run_benchmarks PRIVATE
    benchmark::benchmark_main
    chess_core
    chess_bots
)

if(NOT MSVC)
    target_link_libraries(run_benchmarks PRIVATE -pthread)
endif()

target_compile_options(run_benchmarks PRIVATE ${ENG_FLAGS})
target_link_options(run_benchmarks PRIVATE ${LINK_FLAGS})

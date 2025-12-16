add_executable(unit_tests
    test_bitboard.cpp
    test_position.cpp
    test_zobrist.cpp
    test_move_generation.cpp
    test_search_position.cpp
    test_see.cpp
)
target_link_libraries(unit_tests PRIVATE
    gtest_main
    chess_core
)

add_executable(mate_finding
    test_mate_finding.cpp
)
target_link_libraries(mate_finding PRIVATE
    gtest_main
    chess_core
)

# Register to CTest
gtest_discover_tests(unit_tests)

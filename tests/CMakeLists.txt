add_executable(unit_tests
    test_bitboard.cpp
    test_position.cpp
    test_zobrist.cpp
    test_move_generation.cpp
    test_search_position.cpp
)

target_link_libraries(unit_tests PRIVATE
    gtest_main
    chess_core
)

# Register to CTest
gtest_discover_tests(unit_tests)

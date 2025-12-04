include(CTest)
enable_testing()

add_executable(unit_tests
    test_bitboard.cpp
    test_position.cpp
    test_zobrist.cpp
    test_search_position.cpp
)

target_link_libraries(unit_tests PRIVATE
    gtest_main
    chess_core
    chess_bots
)

# Register to CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)

cmake_minimum_required(VERSION 3.16)
project(ChessBot LANGUAGES CXX VERSION 0.0.1)

# --- Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Options ---
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if (ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fno-omit-frame-pointer)
endif()

if(ENABLE_COVERAGE)
    message(STATUS "Coverage instrumentation enabled")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)

    find_program(LCOV_EXEC lcov)
    find_program(GENHTML_EXEC genhtml)

    if(LCOV_EXEC AND GENHTML_EXEC)
        configure_file(
            ${CMAKE_SOURCE_DIR}/cmake/coverage.sh.in
            ${CMAKE_BINARY_DIR}/coverage.sh
            @ONLY
        )
        file(CHMOD ${CMAKE_BINARY_DIR}/coverage.sh FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
        )
    else()
        message(WARNING "Install lcov for coverage reporting!")
    endif()
endif()

# --- SFML ---
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
)
message(STATUS "Fetching SMFL lib - Version 3.0.2")
FetchContent_MakeAvailable(SFML)

# --- GoogleTest ---
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW ON
)
message(STATUS "Fetching GoogleTest - Version 1.17.0")
FetchContent_MakeAvailable(googletest)


# --- Include directories ---
include_directories(${PROJECT_SOURCE_DIR}/include)

# --- Core library ---
add_library(chess_core
    src/core/types.cpp
    src/core/bitboard.cpp
    src/core/chess.cpp
)

# --- GUI executable ---
add_executable(chess_gui
    src/gui/elements/board_view.cpp
    src/gui/chess_gui.cpp
    src/gui/gui_main.cpp
)
target_link_libraries(chess_gui PRIVATE
    chess_core
    SFML::Graphics
)

# --- Asset copying ---
add_custom_command(TARGET chess_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:chess_gui>/assets)


# --- Tests ---
if (BUILD_TESTS)
    include(CTest)
    enable_testing()
    
    add_executable(run_tests
        tests/test_bitboard.cpp
    )
    target_link_libraries(run_tests PRIVATE
        gtest_main
        chess_core
    )

    # Register to CTest
    include(GoogleTest)
    gtest_discover_tests(run_tests)
endif()

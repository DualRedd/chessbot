cmake_minimum_required(VERSION 3.16)
project(ChessBot LANGUAGES CXX VERSION 0.0.1)

# --- Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Options ---
option(BUILD_TESTS "Build unit tests" ON)

# --- SFML ---
include(FetchContent)
message(STATUS "Fetching SMFL lib - Version 3.0.2")
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SFML)

# --- Include directories ---
include_directories(${PROJECT_SOURCE_DIR}/include)

# --- Core engine library ---
add_library(chess_core
    src/core/game.cpp
    src/core/board.cpp
)
target_link_libraries(chess_core PRIVATE sfml-system)

# --- GUI executable ---
add_executable(chess_gui
    src/main.cpp
    src/interface/sfml_gui.cpp
)

target_link_libraries(chess_gui PRIVATE chess_core SFML::Graphics)

# --- Tests ---
if (BUILD_TESTS)
    enable_testing()
    # TODO
endif()

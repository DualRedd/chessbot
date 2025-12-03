cmake_minimum_required(VERSION 3.16)
project(ChessBot LANGUAGES CXX VERSION 0.0.1)

# --- Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# --- Options ---
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if (ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fno-omit-frame-pointer)
endif()

if(ENABLE_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "Code coverage is not supported on MSVC")
    endif()
    message(STATUS "Coverage instrumentation enabled")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)

    set(COVERAGE_REPORT_DIR ${CMAKE_BINARY_DIR}/reports)

    function(configure_executable_script input output)
        configure_file(${input} ${output} @ONLY)
        file(CHMOD ${output} FILE_PERMISSIONS 
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
        )
    endfunction()

    configure_executable_script(
        ${CMAKE_SOURCE_DIR}/cmake/coverage_report.sh.in
        ${CMAKE_BINARY_DIR}/coverage_report.sh
    )
    configure_executable_script(
        ${CMAKE_SOURCE_DIR}/cmake/clean_coverage.sh.in
        ${CMAKE_BINARY_DIR}/clean_coverage.sh
    )

    find_program(GCOVR_EXEC gcovr)
    if(NOT GCOVR_EXEC)
        message(WARNING "Install gcovr to generate coverage reports!")
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
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

# --- TGUI ---
FetchContent_Declare(TGUI
    GIT_REPOSITORY https://github.com/texus/TGUI.git
    GIT_TAG v1.11.0
    GIT_SHALLOW ON
)
message(STATUS "Fetching TGUI lib - Version 1.11.0")
set(TGUI_BACKEND "SFML_GRAPHICS" CACHE STRING "TGUI backend" FORCE)
set(TGUI_BUILD_GUI_BUILDER OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(TGUI)

# --- GoogleTest ---
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW ON
)
message(STATUS "Fetching GoogleTest - Version 1.17.0")
FetchContent_MakeAvailable(googletest)

# --- GoogleBenchmark ---
if (BUILD_BENCHMARKS)
    FetchContent_Declare(goog_benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.0
        GIT_SHALLOW ON
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(goog_benchmark)
endif()

# --- Include directories ---
include_directories(${PROJECT_SOURCE_DIR}/include)

# --- Core library ---
add_library(chess_core SHARED
    src/core/types.cpp
    src/core/bitboard.cpp
    src/core/position.cpp
    src/core/move_generation.cpp
    src/core/chess.cpp
    src/core/ai_player.cpp
)

# --- AIs ---
add_library(chess_bots SHARED
    src/ai/registry.cpp
    src/ai/search_position.cpp
    src/ai/transposition_table.cpp
    src/ai/ai_random.cpp
    src/ai/ai_minimax.cpp
)
target_link_libraries(chess_bots PRIVATE
    chess_core
)

# --- GUI executable ---
add_executable(chess_gui
    src/gui/gui_main.cpp
    src/gui/chess_gui.cpp
    src/gui/game_manager.cpp
    src/gui/assets.cpp
    src/gui/elements/chess_view.cpp
    src/gui/elements/side_panel_view.cpp
    src/gui/elements/player_config_view.cpp
    src/gui/elements/config_widgets.cpp
)
target_link_libraries(chess_gui PRIVATE
    chess_core
    chess_bots
    SFML::Graphics
    TGUI::TGUI
)

# MSVC specific settings
if(MSVC)
    set_target_properties(chess_gui PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(chess_gui PRIVATE SFML::Main)
    set_target_properties(chess_core PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set_target_properties(chess_bots PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# --- Asset copying ---
add_custom_command(TARGET chess_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:chess_gui>/assets)


# --- Tests ---
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

# -- Benchmarks ---
if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

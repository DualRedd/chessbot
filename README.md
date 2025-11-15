# Shakkitekoäly 

Algoritmit ja tekoäly -kurssin projekti

[![CI](https://github.com/DualRedd/chessbot/actions/workflows/CI.yml/badge.svg)](https://github.com/DualRedd/chessbot/actions/workflows/CI.yml)
[![codecov](https://codecov.io/gh/DualRedd/chessbot/graph/badge.svg?token=7J0XBWX4DI)](https://codecov.io/gh/DualRedd/chessbot)

# Dokumentaatio

[Määrittelydokumentti](https://github.com/DualRedd/chessbot/blob/main/docs/project-spefication.md)

[Testausdokumentti](https://github.com/DualRedd/chessbot/blob/main/docs/testing-report.md)

# Viikkoraportit

[Viikko 1](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-1.md)

[Viikko 2](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-2.md)

[Viikko 3](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-3.md)


# Building the project from source

## Build dependencies

- C++ compiler: GCC or Clang
- CMake for generating build files
- SFML and its [build dependencies](https://www.sfml-dev.org/tutorials/3.0/getting-started/cmake/#requirements) for GUI

Additional:
- gcovr for generating code coverage reports

## Building on Linux

1. Navigate to the root folder and create a build directory:
    ```bash
    mkdir build
    cd build
    ```

2. Generate build files using CMake:
    ```bash
    cmake ..
    ```
    To enable coverage reporting include the flag ```-DENABLE_COVERAGE=ON```. **Note:** coverage instrumentation will affect performance.

3. Build the project:
    ```bash
    cmake --build .
    ```
    **Note:** Use the flag ```-jX``` to enable parallel builds with X threads.

4. Optionally run tests:
    ```bash
    ctest --output-on-failure
    ```
    To create a coverage report run the provided shell script:
    ```bash
    ./coverage_report.sh
    ```
    To clean any previous coverage data run:
    ```bash
    ./clean_coverage.sh
    ```


# Usage

Currently only a GUI interface is provided. To run it:
```bash
./chess_gui
```

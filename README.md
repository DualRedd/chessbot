# Shakkitekoäly 

Algoritmit ja tekoäly -kurssin projekti

[![CI](https://github.com/DualRedd/chessbot/actions/workflows/CI.yml/badge.svg)](https://github.com/DualRedd/chessbot/actions/workflows/CI.yml)
[![codecov](https://codecov.io/gh/DualRedd/chessbot/graph/badge.svg?token=7J0XBWX4DI)](https://codecov.io/gh/DualRedd/chessbot)

# Dokumentaatio

[Määrittelydokumentti](https://github.com/DualRedd/chessbot/blob/main/docs/project-spefication.md)

[Testausdokumentti](https://github.com/DualRedd/chessbot/blob/main/docs/testing-report.md)

[Toteutusdokumentti](https://github.com/DualRedd/chessbot/blob/main/docs/implementation-report.md)

[Käyttöohje](https://github.com/DualRedd/chessbot/blob/main/docs/user-manual.md)

# Viikkoraportit

[Viikko 1](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-1.md)

[Viikko 2](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-2.md)

[Viikko 3](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-3.md)

[Viikko 4](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-4.md)

[Viikko 5](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-5.md)

[Viikko 6](https://github.com/DualRedd/chessbot/blob/main/docs/weekly-reports/week-6.md)

# Building the project from source

## Linux (GCC/Clang)

**Prerequisites**
- C++ compiler: GCC or Clang
- CMake >= 3.16 for generating build files
- SFML [build dependencies](https://www.sfml-dev.org/tutorials/3.0/getting-started/cmake/#requirements)
    ```bash
    sudo apt update
    sudo apt install \
        libxrandr-dev \
        libxcursor-dev \
        libxi-dev \
        libudev-dev \
        libfreetype-dev \
        libflac-dev \
        libvorbis-dev \
        libgl1-mesa-dev \
        libegl1-mesa-dev
    ```

Additional:
- gcovr for generating code coverage reports
    ```bash
    sudo apt update
    sudo apt install gcovr
    ```

**Build steps:**
1. Clone the repository:
    ```bash
    git clone https://github.com/DualRedd/chessbot.git
    cd chessbot
    ```

2. Generate build files using CMake:
    ```bash
    mkdir build
    cd build
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

5. Run the application. Currently only a GUI interface is provided.
    ```bash
    ./chess_gui
    ```


## Windows (MSVC)

Experimental support for MSVC builds is provided. The project is primarily developed and tested on Linux with GCC.

**Prerequisites:**
- Visual Studio 2022 (or newer) with the "Desktop development with C++" workload (or MSVC Build Tools).
- CMake >= 3.16 for generating build files (on PATH).
- Git (required by CMake FetchContent).

**Build steps:**

1. Open "x64 Native Tools Command Prompt for VS 2022" or a PowerShell prompt where MSVC tools are available.

2. Clone the repository:
   ```powershell
   git clone https://github.com/DualRedd/chessbot.git
   cd chessbot
   ```

3. Generate build files using CMake:
    ```powershell
    mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64
    ```
    **Notes:**
    - Code coverage is not supported for MSVC.
    - If you already opened the x64 Native Tools prompt you can omit `-G`/`-A` and run `cmake ..`.
    - Use `-A x64` to ensure 64-bit build. If you use the wrong generator/arch you may get unexpected results.

4. Build the project:
    ```powershell
    cmake --build . --config Release
    ```
    **Note:** Use the flag `-- /m` to enable parallel builds.

5. Optionally run tests:
    ```powershell
    ctest --output-on-failure
    ```

6. Run the application. Currently only a GUI interface is provided.
    ```powershell
    .\Release\chess_gui.exe
    ```
    Or just launch `chess_gui.exe` from File Explorer.

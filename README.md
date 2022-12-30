Picross Solver Library
======================

Picross is a puzzle game licensed by Nintendo. Those puzzles are also called nonograms or
paint-by-number. The goal is to find a hidden picture in a rectangular grid, by painting
some of the cells with one color, or leaving them blank. The information given is, for
each row and each column, the number and size of the segments of continuous filled cells
on that line.

This is a solver library for Picross puzzles. The solver will find the solutions of a grid
based on the row and column constraints given as input. The solver handles grids with
multiple solutions and can be used as a validator to check the uniqueness of the
solution.

## Features

 - The solver library has no dependencies other than C++17
 - Black and white puzzles only
 - Two file formats are supported:
   - Steve Simpson's [NON format](doc/FILE_FORMAT_NON.md)
   - A native format created for this library
 - Battle-tested on a wide range of puzzles: [Performance](doc/PERF.md)

## Library

The Picross solver provided as a library

### Build

With [CMake](https://cmake.org/download/). For example on Windows:

```
mkdir ./build
cd build
cmake -G "Visual Studio 15 2017 Win64" ..
cmake --build . --config Release
```

### Unit Tests

Dependencies:

*  [Catch2](https://github.com/catchorg/Catch2)
*  [PNM++](https://github.com/ToruNiina/pnm)

Build and run unit tests:

```
cmake -G "Visual Studio 15 2017 Win64" -DPICROSS_BUILD_TESTS=ON ..
cmake --build . --target picross_run_utests --config Debug
```

### Install

Install in some dir:

```
cmake --install . --config Release --prefix <some_dir>
```

Or, package the library:

```
cpack -G ZIP -C Release
```

### CMake Integration

The library CMake's build exports the following target: `picross::picross`

### Example

Below is an example of how to use the library.
The API is accessible via a unique header file [`<picross/picross.h>`](src/picross/include/picross/picross.h)

```cpp
#include <picross/picross.h>

#include <cassert>
#include <iostream>

int main()
{
    // Puzzle definition
    const picross::InputGrid::Constraints rows {
        { 3 },
        { 1, 1 },
        { 1, 1 },
        { 3 },
        { 3 },
        { }
    };
    const picross::InputGrid::Constraints cols {
        { },
        { 2 },
        { 2 },
        { 5 },
        { 1 },
        { 3 }
    };
    picross::InputGrid puzzle(rows, cols, "Note");

    // [Optional] Check the puzzle validity
    const auto [check_is_ok, check_msg] = picross::check_input_grid(puzzle);
    assert(check_is_ok);

    // Solve it
    const auto solver = picross::get_ref_solver();
    const auto result = solver->solve(puzzle);
    assert(result.status == picross::Solver::Status::OK);

    // Print out the solution
    assert(result.solutions.size() == 1);
    const auto& solution = result.solutions.front();
    std::cout << solution.grid << std::endl;
}
```

Output:

```
...###
...#.#
...#.#
.###..
.###..
......
```
## Test Applications

A CLI and a GUI applications built on top of the solver library

### Dependencies

Common to all:

* [PNM++](https://github.com/ToruNiina/pnm)

Command line interface:

* [Argagg](https://github.com/vietjtnguyen/argagg)

Graphical UI:

* OpenGL
* [GLFW3](http://glfw.sf.net)
* [Dear ImGui](https://github.com/ocornut/imgui)
* [Portable File Dialogs](https://github.com/samhocevar/portable-file-dialogs)

### Build

On Windows:

```
mkdir ./build
cd build
cmake -G "Visual Studio 15 2017 Win64" -DPICROSS_BUILD_APP=ON -DPICROSS_BUILD_CLI=ON ..
cmake --build . --config Release
```

### Command Line Tool Usage

Run the CLI on the example file:

```
./build/bin/Release/picross_solver_cli.exe inputs/example_input.txt
```

Use the validation mode to test multiple files at once and check the uniqueness of the solution:
 - Output one line per puzzle
 - Status is `OK` if the puzzle is valid and has a unique solution
 - Difficulty hint: `LINE` for puzzles that are line solvable
 - Performance timing

```
./build/bin/Release/picross_solver_cli.exe --validation inputs/PicrossDS/Normal/*
```

### Screenshots of the Graphical Interface

The GUI shows an animation of the solving process, and finally the solution(s). Here are two examples
from the game Picross DS:

![Solution of the Fish and mushroom grids](./doc/img/grid-solutions-fish-and-mushroom.png)

The hardest puzzles are not line solvable, meaning they cannot be solved simply by using the usual
method of iterating on individual rows and columns. When no further progress can be made, the algorithm
has to test different alternatives in order to reach a solution. That backtracking mechanism
is shown with tiles of varying colors:

![Animation of a complex puzzle with backtracking](./doc/img/solver-animation-with-branching.png)
("Mum" puzzle by Jan Wolter: https://webpbn.com/index.cgi?id=65)

## Contributions

This project does not accept pull requests at the moment.

Please [submit an issue](https://github.com/pierre-dejoue/picross-solver/issues/new) for a feature request, a bug report or any question.

## License

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](./LICENSE)



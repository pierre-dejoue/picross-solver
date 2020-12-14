Picross Solver
==============

A text-based Picross puzzle solver in C++

Picross DS is a puzzle game licensed by Nintendo. Those puzzles are sometimes called nonograms.
The goal is to find a hidden picture in a rectangular grid, by painting some of the cells
with one color, or leaving them blank. The information given is, for each row and each
column, the number and sizes of the grousp of continuous filled cells on that line.

This is a solver library for Picross puzzles. The solver will find the solutions of a grid
based on the row and column constraints given as input. It makes use of a backtracking
technique to explore the possible arrangements of filled and empty cells. The solver handles
grids with multiple solutions.

## Building the Library

Build using CMake: https://cmake.org/download/

For example on Windows:

```
mkdir -p build/msvc141x64
cd build/msvc141x64
cmake -G "Visual Studio 15 2017 Win64" ../..
cmake --build . --config Release
```

## Building the Library and the Command Line Tool

On Windows:

```
cmake -G "Visual Studio 15 2017 Win64" -DPICROSS_BUILD_APP=ON ../..
cmake --build . --config Release
```

## Running the Solver

`./build/msvc141x64/bin/Release/picross_solver.exe inputs/example_input.txt`

## License

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](./LICENSE)



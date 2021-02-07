Picross Solver
==============

Picross DS is a puzzle game licensed by Nintendo. Those puzzles are sometimes called nonograms.
The goal is to find a hidden picture in a rectangular grid, by painting some of the cells
with one color, or leaving them blank. The information given is, for each row and each
column, the number and sizes of the grousp of continuous filled cells on that line.

This is a solver library for Picross puzzles. The solver will find the solutions of a grid
based on the row and column constraints given as input. It makes use of a backtracking
technique to explore the possible arrangements of filled and empty cells. The solver handles
grids with multiple solutions.

## Library

The Picross solver provided as a library

### Dependencies

None

### Build

With [CMake](https://cmake.org/download/). For example on Windows:

```
mkdir -p build/msvc141x64
cd build/msvc141x64
cmake -G "Visual Studio 15 2017 Win64" ../..
cmake --build . --config Release
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

## Test Applications

A CLI and a graphical UI applications built on top of the solver library

### Dependencies

Graphical UI:

* OpenGL
* [GLFW3](http://glfw.sf.net)
* [Dear ImGui](https://github.com/ocornut/imgui)
* [Portable File Dialogs](https://github.com/samhocevar/portable-file-dialogs)

### Build

On Windows:

```
mkdir -p build/msvc141x64
cd build/msvc141x64
cmake -G "Visual Studio 15 2017 Win64" -DPICROSS_BUILD_APP=ON -DPICROSS_BUILD_CLI=ON ../..
cmake --build . --config Release
```

### Usage

Run the CLI on an example file:

`./build/msvc141x64/bin/Release/picross_cli.exe inputs/example_input.txt`

## License

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](./LICENSE)



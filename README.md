Picross Solver
==============

A text-based Picross puzzle solver in C++


    Picross is a puzzle game licensed by Nintendo. Those puzzles are sometimes called nonograms.
    The goal is to find a hidden picture in a rectangular grid, by painting some of the cells
    with one color, or leaving them blank. The information given is, for each row and each
    column, the number and sizes of the grousp of continuous filled cells on that line.

    Below is my implementation of a solver for Picross puzzles. I did this as a personnal
    project in 2010. The solver will use a backtracking technique to explore all the possible
    arrangements of filled and empty cells. It will find all solutions of a grid based on the
    row and column constraints given as an input text file. There can be zero, one or several
    solutions.


## License

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](./LICENSE)



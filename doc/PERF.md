Performance
===========

## Introduction

A great survey of nonograms solvers was done by Jan Wolter several years ago, accessible here: [webpbn.com/survey](https://webpbn.com/survey/).

It provides a selection of puzzles of all difficulties (from very easy to almost impossible) that I used for my own testing. I added
a few files to that list, taken from [nonograms.org](https://www.nonograms.org/).

## Test configuration

The methodology follows that of the webpbn survey, that is the Picross Solver is used in validation mode. The goal is to
identify if a puzzle has a unique solution, therefore the number of solutions returned by the solver is limited to 2.

A typical command-line for a single file (but one can also pass multiple files at once to the CLI) would be:

`picross_solver_cli.exe --validation webpbn-00529.non`

Equivalently, the same setup can be obtained with:

`picross_solver_cli.exe --max-nb-solutions 2 webpbn-00529.non`

And will produce a more detailled output than the validation mode.

## Version 0.2.0, 2023/01/28

### New Test Files

The set of test files is part of the release files for `0.2.0`. A few files were added compared to the previous set:

- 5-Dom and 7-Dom test patterns. The previous set only had 9-Dom.
- [Wild Tattoo](https://webpbn.com/play.cgi?id=35105), created by yours truly with the help of the Picross Solver GUI.
- [Tiger](../inputs/qnonograms/tiger.non) is a difficult puzzle that is line-solvable, but requires very long reductions to do so. Depending on the settings of the solver, it may be reported as non-line-solvable (that is the case in the result table below). The CLI has a `--line-solver` option to fully reduce all lines no matter their difficulty.

### Results

Test Hardware: My laptop

```
Processor   Intel(R) Core(TM) i7-8750H CPU @ 2.20GHz
RAM         16 GB
OS          Windows 10
Compiler    MSVC 19.16.27045.0
```

Puzzles sorted by solving time:

|File                   |Grid       |Size |Status|Difficulty|Timing     |
|-----------------------|-----------|-----|------|----------|-----------|
|webpbn-00001.non       |Dance      |5x10 |OK    |LINE      |0.05 ms    |
|webpbn-00023.non       |Edge       |10x11|OK    |          |0.20 ms    |
|webpbn-00021.non       |Skid       |14x25|OK    |LINE      |0.28 ms    |
|webpbn-00006.non       |Cat        |20x20|OK    |LINE      |0.58 ms    |
|webpbn-00027.non       |Bucks      |27x23|OK    |          |0.80 ms    |
|webpbn-02413.non       |Smoke      |20x20|OK    |          |1.59 ms    |
|nonograms-org-4010.txt |Jelly-fish |33x40|OK    |LINE      |2.14 ms    |
|webpbn-00529.non       |Swing      |45x45|OK    |LINE      |4.72 ms    |
|pattern-5-dom.txt      |5-Dom      |11x11|OK    |          |5.9 ms     |
|webpbn-00016.non       |Knot       |34x34|OK    |LINE      |9.56 ms    |
|nonograms-org-42735.txt|Wolverine  |40x40|OK    |LINE      |51.1 ms    |
|webpbn-35105.non       |Wild Tattoo|60x45|OK    |          |62.4 ms    |
|webpbn-02556.non       |Flag       |65x45|MULT  |          |62.5 ms    |
|webpbn-00436.non       |Petroglyph |40x35|OK    |          |76.3 ms    |
|qnonograms-sun.non     |Sun        |50x60|OK    |LINE      |135.4 ms   |
|webpbn-00065.non       |Mum        |34x40|OK    |          |167.6 ms   |
|webpbn-34824.non       |Wishbone   |39x47|OK    |          |223.8 ms   |
|pattern-7-dom.txt      |7-Dom      |15x15|OK    |          |275.8 ms   |
|webpbn-01694.non       |Tragic     |45x50|OK    |          |556.7 ms   |
|webpbn-06574.non       |Forever    |25x25|OK    |          |1.3 s      |
|webpbn-07604.non       |No DiCaprio|55x55|OK    |LINE      |4.3 s      |
|nonograms-org-3016.txt |Barque     |50x60|OK    |LINE      |4.6 s      |
|webpbn-00803.non       |Lighthouse |50x45|OK    |          |9.75 s     |
|webpbn-01611.non       |Merka      |55x60|OK    |          |10.2 s     |
|webpbn-04645.non       |Marilyn    |50x70|OK    |          |22.3 s     |
|qnonograms-tiger.non   |Tiger      |75x50|OK    |          |36.6 s     |
|webpbn-03541.non       |Sign       |60x50|OK    |          |44.3 s     |
|webpbn-08098.non       |9-Dom      |19x19|OK    |          |57.6 s     |
|webpbn-10810.non       |Centerpiece|60x60|MULT  |          |93.6 s     |
|webpbn-02712.non       |Lion       |47x47|MULT  |          |112 s      |
|webpbn-06739.non       |Karate     |40x40|MULT  |          |259 s      |
|webpbn-02040.non       |Hot        |55x60|OK    |          |51 m       |
|webpbn-09892.non       |Nature     |50x40|      |          |+          |
|webpbn-10088.non       |Marley     |52x63|      |          |+          |
|webpbn-12548.non       |Sierpinsky |47x40|      |          |+          |
|webpbn-18297.non       |Thing      |36x42|      |          |+          |
|webpbn-22336.non       |Gettys     |99x59|      |          |+          |

- **Status:** `OK` if the solution is unique. `MULT` if there are multiple solutions. Nothing if the solving process was aborted.
- **Difficulty:** `LINE` if the grid is line solvable. Else, it requires to make hypothesis on at least one line.
- **Timing:** `+` indicates the solver was interrupted because it was too long (usually after 1 hour).

### Conclusion

The performance has improved on most puzzles, and the solver is now able to tackle some of the hardest puzzles of the set (Lion, Karate and Hot, which version `0.1.0` could not solve in a reasonnable amount of time).

- For the most part the improvements are due to the new "partial reduction" step in the solver which quickly discover some of the tiles of a line without doing a full reduction. This allows for very fast progress on the easy parts of a puzzle. The easiest puzzles of the test set are in fact solved solely thanks to that method.
- The new "probing" method is a key asset to tackle the hardest puzzles. It consists in a search step limited to depth D+1, and done on the edges of the currently unsolved part of a grid. All tiles identified thanks to that method are set in the grid at depth D, and the line solver resumes from that point on. The most spectacular demonstration of that method is with the puzzle Forever, which used to take hours to solve and is now solved in a few seconds.
- Unfortunately on some puzzles the "probing" method will consume a lot of CPU time for nothing, since it will not allow to make any progress. This is the case with the most geometric patterns, like the Domino test pattern (9-Dom for instance), and Centerpiece. Disabling the "probing" step in the solver, version 0.2.0 is able to solve Centerpiece (i.e. find at least 2 solutions) in less than a second.

## Version 0.1.0, 2021/05/03

### Results

Test Hardware: My laptop

```
Processor   Intel(R) Core(TM) i7-8750H CPU @ 2.20GHz
RAM         16 GB
OS          Windows 10
Compiler    MSVC 19.28.29914.0
```

Puzzles sorted by solving time:

|File                   |Grid       |Size |Status|Difficulty|Timing     |
|-----------------------|-----------|-----|------|----------|-----------|
|webpbn-00001.non       |Dance      |5x10 |OK    |LINE      |0.05 ms    |
|webpbn-00021.non       |Skid       |14x25|OK    |LINE      |0.21 ms    |
|webpbn-00023.non       |Edge       |10x11|OK    |          |0.26 ms    |
|webpbn-00006.non       |Cat        |20x20|OK    |LINE      |0.36 ms    |
|webpbn-00027.non       |Bucks      |27x23|OK    |          |0.59 ms    |
|webpbn-02413.non       |Smoke      |20x20|OK    |          |2.81 ms    |
|webpbn-00016.non       |Knot       |34x34|OK    |LINE      |9.35 ms    |
|webpbn-02556.non       |Flag       |65x45|MULT  |          |9.38 ms    |
|nonograms-org-4010.txt |Jelly-fish |33x40|OK    |LINE      |13.4 ms    |
|webpbn-00529.non       |Swing      |45x45|OK    |LINE      |13.4 ms    |
|webpbn-00436.non       |Petroglyph |40x35|OK    |          |107.4 ms   |
|nonograms-org-42735.txt|Wolverine  |40x40|OK    |LINE      |311.3 ms   |
|webpbn-00065.non       |Mum        |34x40|OK    |          |522.6 ms   |
|webpbn-01694.non       |Tragic     |45x50|OK    |          |1.172 s    |
|webpbn-00803.non       |Lighthouse |50x45|OK    |          |3.778 s    |
|webpbn-07604.non       |No DiCaprio|55x55|OK    |LINE      |4.340 s    |
|webpbn-10810.non       |Centerpiece|60x60|MULT  |          |18.236 s   |
|nonograms-org-3016.txt |Barque     |50x60|OK    |LINE      |23.195 s   |
|webpbn-08098.non       |9-Dom      |19x19|OK    |          |67.585 s   |
|webpbn-04645.non       |Marilyn    |50x70|OK    |          |137.72 s   |
|webpbn-03541.non       |Sign       |60x50|OK    |          |159.9 s    |
|webpbn-01611.non       |Merka      |55x60|OK    |          |167.5 s    |
|webpbn-06574.non       |Forever    |25x25|OK    |          |2.5 h      |
|webpbn-02040.non       |Hot        |55x60|      |          |+          |
|webpbn-02712.non       |Lion       |47x47|      |          |+          |
|webpbn-06739.non       |Karate     |40x40|      |          |+          |
|webpbn-09892.non       |Nature     |50x40|      |          |+          |
|webpbn-10088.non       |Marley     |52x63|      |          |+          |
|webpbn-12548.non       |Sierpinsky |47x40|      |          |+          |
|webpbn-18297.non       |Thing      |36x42|      |          |+          |
|webpbn-22336.non       |Gettys     |99x59|      |          |+          |

- **Status:** `OK` if the solution is unique. `MULT` if there are multiple solutions. Nothing if the solving process was aborted.
- **Difficulty:** `LINE` if the grid is line solvable. Else, it requires to make hypothesis on at least one line.
- **Timing:** `+` indicates the solver was manually interrupted because it was too long (usually after 1 hour).

### Memory

Memory usage is very low, the CLI process stays below 1MB in validation mode for the puzzles listed above.

Using the library in a more general context, one has to be careful to limit the max number of solutions (in the case of a validation,
it is limited to 2 solutions) because some ill-formed puzzles can have a high number of solutions and the solver will allocate a huge
quantity of memory to store them all. One such example is `Centerpiece` if the solver is used with no limit on the number of solutions:

`picross_solver_cli.exe webpbn-10810.non` :warning:

### Conclusion

My "end game" at the moment is the very well named puzzle `Forever` ([webpbn #6574](https://webpbn.com/play.cgi?id=6574)) for which the Picross Solver correctly finds the unique solution,
albeit after several hours of processing!

The robustness of the Picross Solver is satisfactory in the sense that it correctly finds the solution(s) of a wide variety of puzzles, even hard ones.
The limiting factor on the most difficult puzzles is really the processing time.

By design the solver will not start a search (i.e. test several hypothesis with a backtracking technique) before it reaches a state where no further
progress can be made with line solving alone. This has the advantage that the solver is good at detecting if a puzzle is line solvable (the label `LINE` in the table above).
But it comes at a cost performance wise. In some examples like `Barque`, which is line solvable, it would probably be more advantageous to do a search on a line with a small
number of alternatives rather than exploring the huge solution space of a few lines.
The same should stay true for puzzles that are not line solvable but have a unique solution.

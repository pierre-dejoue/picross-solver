NIN File Format
===============

The NIN file format used by [Jakub Wilk's nonogram solver](https://github.com/jwilk-archive/nonogram) program

## Description

This is a text file where the first line gives the width and height of the puzzle,
and the next lines gives the constraints, first for the rows (top to bottom), then
the columns (left to right).

```
10 10               <--- Width and height
1 2 3               <--- Constraint on the top row
1 1                 <--- Constraint on the next row
...
3 2                 <--- Constraint on the left column
2 2 1               <--- Constraint on the next column
...
```

Write `0` for a blank row or column.

## An example

```
$ nonogram <<EOF
6 3
3 2
1 2 1
2 3
3
1 1
2
2
1 1
3
EOF
        3 1 2 2 1 3
          1     1
      +------------+
 3 2  |######  ####|
 1 2 1|##  ####  ##|
 2 3  |####  ######|
      +------------+
```

Other examples of puzzles using this format: https://github.com/jwilk-archive/nonogram/tree/master/data

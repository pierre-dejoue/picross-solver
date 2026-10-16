NON File Format
===============

## Description

The NON file format for nonogram puzzles was originally developped by Steve Simpson.

It has been extended to support colored nonograms.

Here are two resources that describe the format:
 - Orignal version: [Puzzle-file format](https://stevocity.me.uk/nonogram/fmt2)
 - Extended version: [NON File Format](https://github.com/Sprunth/nonogram-db/blob/master/FORMAT.md)

## An example

The NON file:
```
catalogue "webpbn.com #1"
title "Dancer"
by "Jan Wolter"
copyright "&copy; Copyright 2004 by Jan Wolter"
width 5
height 10

rows
2
2,1
1,1
3
1,1
1,1
2
1,1
1,2
2

columns
2,1
2,1,3
7
1,3
2,1

goal "01100011010010101110101001010000110010100101111000"
```

The puzzle:

```
     2 2 7 1 2
     1 1   3 1
       3
    +----------+
2   |  ####    |
2 1 |  ####  ##|
1 1 |    ##  ##|
3   |  ######  |
1 1 |##  ##    |
1 1 |##  ##    |
2   |    ####  |
1 1 |  ##  ##  |
1 2 |  ##  ####|
2   |####      |
    +----------+
```

## Caveats

It is not clear in both descriptions if a blank line should be represented by an empty line or a line
with a zero. The Picross solver supports both representations.

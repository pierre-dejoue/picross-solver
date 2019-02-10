/*******************************************************************************
 * PICROSS SOLVER: picross_solver.h
 *
 *  Declaration of the classes of the Picross solver.
 *
 * Author/Copyright: Pierre DEJOUE - pdejoue.perso.neuf.fr - 2010
 ******************************************************************************/
#ifndef PICROSS_SOLVER_H
#define PICROSS_SOLVER_H

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <functional>

using namespace std;

// Defined at the end of the file
class  GridInput;
struct GridStats;

/*
 * Exceptions specific to the Picross solver:
 *  - PicrossLineAdditionError
 *  - PicrossGridCannotBeSolved
 */

class PicrossLineAdditionError: public exception
{
    const char * what () const throw () { return "Picross Solver: an error was detected adding two lines."; }
};

class PicrossGridCannotBeSolved: public exception
{
    const char * what () const throw () { return "Picross Solver: this is a warning indicating that the current grid cannot be solved."; }
};

/*
 * Tile namespace.
 *
 *   A tile is the base element to construct lines and grids: it can be empty (ZERO) of filled (ONE).
 *   The following namespace defines the constants, types and functions used for the manipulation of tiles.
 */
namespace Tile
{
    const int UNKNOWN = -1, ZERO = 0, ONE = 1;
    typedef int tile;

    char str(tile t);
    tile add(tile t1, tile t2);
    tile reduce(tile t1, tile t2);
}

/*
 * Line class.
 *
 *   A line can be either a row or a column of a grid. It consists of an array of tiles.
 */
class Line
{
public:
    enum Type { ROW = 0, COLUMN };
private:
    Type type;
    vector<Tile::tile> tiles;
public:
    Line(Type type, vector<Tile::tile>& vect);
    Line(const Line& line);                                  // copy constructor
    Type       get_type()        const { return type; }
    int        get_size()        const { return tiles.size(); }
    Tile::tile get_tile(int idx) const { return tiles.at(idx); }
    vector<Tile::tile>::const_iterator read_tiles() const;   // iterator to read the tiles.
    bool is_all_one_color(Tile::tile color) const;
    void add(const Line& line);
    void reduce(const Line& line);
    void print() const;
};

//Line         add_two_lines(const Line& line1, const Line& line2);
void         add_and_filter_lines(list<Line>& list_of_lines, const Line& filter_line, GridStats * stats);
Line         reduce_lines(const list<Line>& list_of_lines, GridStats * stats);
string       str_line(const Line& line);
string       str_line_type(Line::Type type);

/*
 * Grid class.
 *
 *   Describe a full puzzle grid. This is also the working class used to solve it (Grid::solve())
 */
class Grid {
private:
    Tile::tile**       grid;            // 2D array of tiles.
    int                width, height;
    const GridInput&   grid_input;      // reference to the constraints information
    vector<bool>       line_complete[2];
    vector<bool>       line_to_be_reduced[2];  // flag added for optimization
    vector<int>        alternatives[2];
    Line::Type         guess_line_type;
    int                guess_line_index;
    list<Line>         guess_list_of_all_alternatives;
    vector<Grid>&      saved_solutions; // reference to a vector where to store solutions
    GridStats*         stats;           // if not null, the program will store some stats in that structure
    int                nested_level;    // nested_level is incremented by function Grid::guess()
public:
    Grid(int width, int height, const GridInput& grid_input, vector<Grid>& solutions, GridStats* in_stats = 0, int n_level = 0);
    Grid& operator=(const Grid& c_grid);
    Grid(const Grid& c_grid);
    ~Grid();
    Line get_line(Line::Type type, int index);
    bool set_line(Line line, int index);
    bool reduce_one_line(Line::Type type, int index);
    bool reduce_all_rows_and_columns();
    bool is_solved();
    bool solve();
    void print();
private:
    bool set(int x, int y, Tile::tile t);
    bool guess();
    void save_solution();
};

/*
 * Constraint class.
 *
 *   It defines the constraints that apply to one line (row or column) in a grid.
 *   It provides the size of each of the groups of contiguous filled tiles.
 *   This is basically the input information of the solver
 */
class Constraint {
private:
    Line::Type  type;                                // Row or column
    vector<int> sets_of_ones;                        // Size of the continuing blocks of filled tiles
    vector<int> sets_of_ones_plus_trailing_zero;
    int min_line_size;
public:
    Constraint(Line::Type type, const vector<int>& vect);
    int        nb_filled_tiles() const;                           // Total number of filled tiles
    int        nb_blocks() const { return sets_of_ones.size(); }
    int        max_block_size() const;                            // Max size of a group of contiguous filled tiles
    int        get_min_line_size() const { return min_line_size; }
    void       print() const;
    int        theoretical_nb_alternatives(int size, GridStats * stats) const;
    list<Line> build_all_possible_lines_with_size(int size, const Line& filter_line, GridStats * stats) const;
};

/*
 * GridInput class.
 *
 *   A data structure to store the information related to one grid and coming from the input file.
 */
class GridInput {
public:
    string             name;
    vector<Constraint> rows;
    vector<Constraint> columns;
    GridInput() { }
    bool sanity_check();
};

/*
 * GridStats struct.
 *
 *   A data structure to store some stat data.
 */
struct GridStats {
    int     guess_max_nested_level;
    int     guess_total_calls;
    int     guess_max_alternatives;
    int     guess_total_alternatives;
    int     nb_reduce_lines_calls;
    int     max_reduce_list_size;
    int     max_theoretical_nb_alternatives;
    int     total_lines_reduced;
    int     nb_add_and_filter_calls;
    int     max_add_and_filter_list_size;
    int     total_lines_added_and_filtered;
    int     nb_reduce_all_rows_and_colums_calls;
};

void reset_grid_stats(GridStats * stats);
void print_grid_stats(GridStats * stats);

#endif // PICROSS_SOLVER_H


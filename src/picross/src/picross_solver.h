/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#pragma once


#include <list>
#include <memory>
#include <string>
#include <vector>


#include <picross/picross.h>


namespace picross
{

/*
 * Tile namespace.
 *
 *   A tile is the base element to construct lines and grids: it can be empty (ZERO) of filled (ONE).
 *   The following namespace defines the constants, types and functions used for the manipulation of tiles.
 */
namespace Tile
{
    char str(Type t);
    Type add(Type t1, Type t2);
    Type reduce(Type t1, Type t2);
}


/*
 * Line class
 *
 *   A line can be either a row or a column of a grid. It consists of an array of tiles.
 */
class Line
{
public:
    using Type = unsigned int;
    static constexpr Type ROW = 0u, COLUMN = 1u;
public:
    Line(Type type, const std::vector<Tile::Type>& vect);
public:
    Type get_type() const { return type; }
    size_t get_size() const { return tiles.size(); }
    Tile::Type get_tile(int idx) const { return tiles.at(idx); }
    const std::vector<Tile::Type>& get_tiles() const;
    std::vector<Tile::Type>::const_iterator cbegin() const;
    std::vector<Tile::Type>::const_iterator cend() const;
    bool is_all_one_color(Tile::Type color) const;
    void add(const Line& line);
    void reduce(const Line& line);
    void print() const;
private:
    Type type;
    std::vector<Tile::Type> tiles;
};


void add_and_filter_lines(std::list<Line>& list_of_lines, const Line& filter_line, GridStats* stats);
Line reduce_lines(const std::list<Line>& list_of_lines, GridStats* stats);
std::string str_line(const Line& line);
std::string str_line_type(Line::Type type);


/*
 * Constraint class
 */
class Constraint
{
public:
    Constraint(Line::Type type, const InputConstraint& vect);
public:
    unsigned int nb_filled_tiles() const;                       // Total number of filled tiles
    size_t nb_blocks() const { return sets_of_ones.size(); }
    unsigned int max_block_size() const;                        // Max size of a group of contiguous filled tiles
    unsigned int get_min_line_size() const { return min_line_size; }
    void print() const;
    int theoretical_nb_alternatives(unsigned int line_size, GridStats * stats) const;
    std::list<Line> build_all_possible_lines_with_size(unsigned int line_size, const Line& filter_line, GridStats * stats) const;
private:
    Line::Type type;                                            // Row or column
    InputConstraint sets_of_ones;                               // Size of the continuing blocks of filled tiles
    unsigned int min_line_size;
};


/*
 * Grid class
 *
 *   Describe a full puzzle grid. This is also the working class used to solve it (Grid::solve())
 */
class Grid : public SolvedGrid
{
public:
    Grid(const InputGrid& grid, std::vector<std::unique_ptr<SolvedGrid>>& solutions, GridStats* stats = nullptr);
    Grid(const Grid& other);
    Grid& operator=(const Grid& other);
    virtual ~Grid();
private:
    Grid(const std::string& grid_name, const std::vector<Constraint>& rows, const std::vector<Constraint>& columns, std::vector<std::unique_ptr<SolvedGrid>>& solutions, GridStats* stats, unsigned int nested_level = 0u);
    Grid(const Grid& parent, unsigned int nested_level);
public:
    Line get_line(Line::Type type, unsigned int index) const;
    bool is_solved() const;
    bool solve();
    const std::string& get_name() const override;
    unsigned int get_height() const override;
    unsigned int get_width() const override;
    std::vector<Tile::Type> get_row(unsigned int index) const override;
    void print() const override;
private:
    bool set_line(Line line, unsigned int index);
    bool reduce_one_line(Line::Type type, unsigned int index);
    bool reduce_all_rows_and_columns();
    bool set(unsigned int x, unsigned int y, Tile::Type t);
    bool guess();
    void save_solution();
private:
    unsigned int                                height, width;
    std::string                                 grid_name;
    std::vector<Constraint>                     rows;
    std::vector<Constraint>                     columns;
    std::vector<std::unique_ptr<SolvedGrid>>&   saved_solutions; // reference to a vector where to store solutions
    Tile::Type**                                grid;            // 2D array of tiles.
    GridStats*                                  stats;           // if not null, the program will store some stats in that structure
    std::vector<bool>                           line_complete[2];
    std::vector<bool>                           line_to_be_reduced[2];  // flag added for optimization
    std::vector<unsigned int>                   alternatives[2];
    Line::Type                                  guess_line_type;
    unsigned int                                guess_line_index;
    std::list<Line>                             guess_list_of_all_alternatives;
    unsigned int                                nested_level;    // nested_level is incremented by function Grid::guess()
};


/*
 * Grid solver: an implementation
 */
class RefSolver : public Solver
{
public:
    std::vector<std::unique_ptr<SolvedGrid>> solve(const InputGrid& grid_input, GridStats* stats) const override;
};

} // namespace picross

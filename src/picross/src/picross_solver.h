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
#include <ostream>
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
    static constexpr Type ROW = 0u, COL = 1u;
public:
    Line(Type type, const std::vector<Tile::Type>& tiles);
    Line(Type type, std::vector<Tile::Type>&& tiles);
    Line(Type type, const OutputGrid& grid, size_t index);
public:
    Type get_type() const { return type; }
    unsigned int size() const { return static_cast<unsigned int>(tiles.size()); }
    Tile::Type get_tile(int idx) const { return tiles.at(idx); }
    const std::vector<Tile::Type>& get_tiles() const;
    std::vector<Tile::Type>::const_iterator cbegin() const;
    std::vector<Tile::Type>::const_iterator cend() const;
    bool is_all_one_color(Tile::Type color) const;
    void add(const Line& line);
    void reduce(const Line& line);
    void print(std::ostream& ostream) const;
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
    Constraint(Line::Type type, const InputGrid::Constraint& vect);
public:
    unsigned int nb_filled_tiles() const;                       // Total number of filled tiles
    size_t nb_blocks() const { return sets_of_ones.size(); }
    unsigned int max_block_size() const;                        // Max size of a group of contiguous filled tiles
    unsigned int get_min_line_size() const { return min_line_size; }
    void print(std::ostream& ostream) const;
    int theoretical_nb_alternatives(unsigned int line_size, GridStats * stats) const;
    std::list<Line> build_all_possible_lines(const Line& filter_line, GridStats * stats) const;
private:
    Line::Type type;                                            // Row or column
    InputGrid::Constraint sets_of_ones;                         // Size of the continuing blocks of filled tiles
    unsigned int min_line_size;
};


/*
 * WorkGrid class
 *
 *   Working class used to solve a grid.
 */
class WorkGrid final : public OutputGrid
{
public:
    WorkGrid(const InputGrid& grid, Solver::Solutions* solutions, GridStats* stats = nullptr);
    WorkGrid(const WorkGrid& other) = delete;
    WorkGrid& operator=(const WorkGrid& other) = delete;
    WorkGrid(WorkGrid&& other) = default;
    WorkGrid& operator=(WorkGrid&& other) = default;
private:
    WorkGrid(const WorkGrid& parent, unsigned int nested_level);
public:
    bool solve();
private:
    bool all_lines_completed() const;
    bool set_line(Line line, unsigned int index);
    bool reduce_one_line(Line::Type type, unsigned int index);
    bool reduce_all_lines();
    bool set_w_reduce_flag(size_t x, size_t y, Tile::Type t);
    bool guess() const;
    void save_solution() const;
private:
    std::vector<Constraint>                     rows;
    std::vector<Constraint>                     cols;
    Solver::Solutions*                          saved_solutions; // ptr to a vector where to store solutions
    GridStats*                                  stats;           // if not null, the program will store some stats in that structure
    std::vector<bool>                           line_completed[2];
    std::vector<bool>                           line_to_be_reduced[2];  // flag added for optimization
    std::vector<unsigned int>                   nb_alternatives[2];
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
    Solver::Solutions solve(const InputGrid& grid_input, GridStats* stats) const override;
};

} // namespace picross

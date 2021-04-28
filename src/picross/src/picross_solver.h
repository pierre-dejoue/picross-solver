/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#pragma once


#include <picross/picross.h>


#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>


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
    Type delta(Type t1, Type t2);
    Type reduce(Type t1, Type t2);
}


/*
 * Line related functions
 */
bool is_all_one_color(const Line& line, Tile::Type color);
void add_and_filter_lines(std::vector<Line>& lines, const Line& known_tiles, GridStats* stats);
std::string str_line(const Line& line);
std::string str_line_type(Line::Type type);
Line line_delta(const Line& line1, const Line& line2);


/*
 * Constraint class
 */
class Constraint
{
public:
    Constraint(Line::Type type, const InputGrid::Constraint& vect);
public:
    unsigned int nb_filled_tiles() const;                       // Total number of filled tiles
    size_t nb_segments() const { return segs_of_ones.size(); }  // Number of segments of contiguous filled tiles
    unsigned int max_segment_size() const;                      // Max segment size
    unsigned int get_min_line_size() const { return min_line_size; }
    int theoretical_nb_alternatives(unsigned int line_size, GridStats * stats) const;
    std::vector<Line> build_all_possible_lines(const Line& known_tiles, GridStats * stats) const;
    std::pair<Line, unsigned int> reduce_and_count_alternatives(const Line& filter_line, GridStats * stats) const;
    void print(std::ostream& ostream) const;
private:
    Line::Type type;                                            // Row or column
    InputGrid::Constraint segs_of_ones;                         // Size of the contiguous blocks of filled tiles
    unsigned int min_line_size;
};


std::ostream& operator<<(std::ostream& ostream, const Constraint& constraint);


/*
 * WorkGrid class
 *
 *   Working class used to solve a grid.
 */
class WorkGrid final : public OutputGrid
{
public:
    WorkGrid(const InputGrid& grid, Solver::Solutions* solutions, GridStats* stats = nullptr, Solver::Observer observer = Solver::Observer());
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
    bool set_line(const Line& line);
    bool single_line_pass(Line::Type type, unsigned int index);
    bool full_grid_pass();
    bool set_w_reduce_flag(size_t x, size_t y, Tile::Type t);
    bool guess() const;
    void save_solution() const;
private:
    std::vector<Constraint>                     rows;
    std::vector<Constraint>                     cols;
    Solver::Solutions*                          saved_solutions; // ptr to a vector where to store solutions
    GridStats*                                  stats;           // if not null, the solver will store some stats in that structure
    Solver::Observer                            observer;        // if not empty, the solver will notify the observer of its progress
    std::vector<bool>                           line_completed[2];
    std::vector<bool>                           line_to_be_reduced[2];
    std::vector<unsigned int>                   nb_alternatives[2];
    std::vector<Line>                           guess_list_of_all_alternatives;
    unsigned int                                nested_level;    // nested_level is incremented by function Grid::guess()
};


/*
 * Grid solver: an implementation
 */
class RefSolver : public Solver
{
public:
    Solver::Solutions solve(const InputGrid& grid_input, GridStats* stats) const override;
    void set_observer(Observer observer) override;
private:
    Observer observer;
};

} // namespace picross

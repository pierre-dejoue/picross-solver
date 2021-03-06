/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - Data structures
 *   - Solver
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#pragma once


#include <stddef.h>

#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>


namespace picross
{

/*
 * Tile namespace
 *
 *   A tile is the base element to construct lines and grids: it can be empty (ZERO) of filled (ONE).
 *   The following namespace defines the constants, types and functions used for the manipulation of tiles.
 */
namespace Tile
{
    using Type = int;
    constexpr Type UNKNOWN = -1, ZERO = 0, ONE = 1;
}


/*
 * InputGrid class
 *
 *   A data structure to store the information related to an unsolved Picross grid.
 *   This is basically the input information of the solver.
 */
struct InputGrid
{
    // Constraint that applies to a line (row or column)
    // It gives the size of each of the groups of contiguous filled tiles
    using Constraint = std::vector<unsigned int>;

    std::string name;
    std::vector<Constraint> rows;
    std::vector<Constraint> cols;
};


std::pair<bool, std::string> check_grid_input(const InputGrid& grid_input);


/*
 * GridStats struct
 *
 *   A data structure to store some stat related to the solver (optional).
 */
struct GridStats
{
    unsigned int max_nested_level = 0u;
    unsigned int guess_total_calls = 0u;
    unsigned int guess_max_alternatives = 0u;
    unsigned int guess_total_alternatives = 0u;
    unsigned int nb_reduce_line_calls = 0u;
    unsigned int max_reduce_list_size = 0u;
    unsigned int max_theoretical_nb_alternatives = 0u;
    unsigned int total_lines_reduced = 0u;
    unsigned int nb_add_and_filter_calls = 0u;
    unsigned int max_add_and_filter_list_size = 0u;
    unsigned int total_lines_added_and_filtered = 0u;
    unsigned int nb_full_grid_pass_calls = 0u;
    unsigned int nb_single_line_pass_calls = 0u;
};


std::ostream& operator<<(std::ostream& ostream, const GridStats& stats);


/*
 * Line class
 *
 *   A line can be either a row or a column of a grid. It consists of an array of tiles.
 */
class Line
{
public:
    using Type = size_t;
    static constexpr Type ROW = 0u, COL = 1u;
public:
    Line(Type type, const std::vector<Tile::Type>& tiles);
    Line(Type type, std::vector<Tile::Type>&& tiles);
public:
    Type get_type() const;
    const std::vector<Tile::Type>& get_tiles() const;
    size_t size() const;
    Tile::Type at(size_t idx) const;
    void add(const Line& line);
    void reduce(const Line& line);
private:
    Type type;
    std::vector<Tile::Type> tiles;
};


std::ostream& operator<<(std::ostream& ostream, const Line& line);


/*
 * OutputGrid class
 *
 *   A partially or fully solved Picross grid
 */
class OutputGrid
{
public:
    OutputGrid(size_t width, size_t height, const std::string& name = "");
    OutputGrid(const OutputGrid& other) = default;
    OutputGrid(OutputGrid&& other) = default;
    OutputGrid& operator=(const OutputGrid& other);
    OutputGrid& operator=(OutputGrid&& other);

    const std::string& get_name() const;
    size_t get_width() const;
    size_t get_height() const;

    Tile::Type get(size_t x, size_t y) const;
    void set(size_t x, size_t y, Tile::Type t);

    Line get_line(Line::Type type, size_t index) const;

    bool is_solved() const;
private:
    const size_t                                width, height;
    const std::string                           name;
    std::vector<Tile::Type>                     grid;            // 2D array of tiles
};


std::ostream& operator<<(std::ostream& ostream, const OutputGrid& grid);


/*
 * Grid solver interface
 */
class Solver
{
public:
    virtual ~Solver() = default;

    //
    // Main method called to solve a grid
    //
    using Solutions = std::vector<OutputGrid>;

    virtual Solutions solve(const InputGrid& grid_input, GridStats* stats = nullptr) const = 0;

    //
    // It is possible to set an observer on the solver. That object will be notified when certain
    // events happen, allowing the user to follow the process step by step.
    //
    // The observer is a function object with the following signature:
    //
    // void observer(Event event, const Line* delta, unsigned int index, unsigned int depth);
    //
    //  * event = DELTA_LINE        delta != nullptr            index is set        depth is set
    //
    //      A line of the output grid has been updated, the delta between the previous value of that line
    //      and the new one is given in delta. The index is the index of the row or column in the grid,
    //      depending on the type of the delta line.
    //      The depth is set to zero initially, then it is the same value as that of the last BRANCHING event.
    //
    //  * event = BRANCHING         delta = nullptr             index = 0u          depth > 0
    //
    //      This event occurs when the algorithm is branching between several alternative solutions, or
    //      when it is going back to an earlier branch. Upon starting a new branch the depth is increased
    //      by one. In the other case, depth is set the the value of the earlier branch.
    //      NB: The user must keep track of the state of the grid at each branching point in order to be
    //          able to reconstruct the final solutions based on the observer events only.
    //      NB: There is no BRANCHING event sent at the beginning of the solving process (depth = 0),
    //          therefore the depth of the first BRANCHING event is always 1.
    //
    //  * event = SOLVED_GRID       delta = nullptr             index = 0u          depth is set
    //
    //      A solution grid has been found. The sum of all the delta lines up until that stage is the solved grid.
    //
    enum class Event { DELTA_LINE, BRANCHING, SOLVED_GRID };
    using Observer = std::function<void(Event,const Line*,unsigned int,unsigned int)>;

    virtual void setObserver(Observer observer) = 0;
};


std::ostream& operator<<(std::ostream& ostream, Solver::Event event);


/*
 * Factory for the reference grid solver
 */
std::unique_ptr<Solver> get_ref_solver();

} // namespace picross

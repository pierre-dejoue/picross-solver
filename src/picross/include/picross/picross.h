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
#include <map>
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

    std::map<std::string, std::string> metadata;      // Optional
};


/*
 * Return the grid size as a string "WxH" with W the width and H the height
 */
std::string grid_size_str(const InputGrid& grid);


/*
 * Sanity check of the constraints of the input grid:
 *
 * - Non-zero height and width
 * - Same number of filled tiles on the rows and columns
 * - The height and width are sufficient to cope with the individual constraints
 */
std::pair<bool, std::string> check_grid_input(const InputGrid& grid);


/*
 * GridStats struct
 *
 *   A data structure to store some stat related to the solver (optional).
 */
struct GridStats
{
    unsigned int nb_solutions = 0u;
    unsigned int max_nb_solutions = 0u;
    unsigned int max_nested_level = 0u;
    unsigned int guess_total_calls = 0u;
    unsigned int guess_total_alternatives = 0u;
    std::vector<unsigned int> guess_max_nb_alternatives_by_depth;
    unsigned int max_initial_nb_alternatives = 0u;
    unsigned int max_nb_alternatives = 0u;
    unsigned int max_nb_alternatives_w_change = 0u;
    unsigned int nb_reduce_list_of_lines_calls = 0u;
    unsigned int max_reduce_list_size = 0u;
    unsigned int total_lines_reduced = 0u;
    unsigned int nb_reduce_and_count_alternatives_calls = 0u;
    unsigned int nb_full_grid_pass_calls = 0u;
    unsigned int nb_single_line_pass_calls = 0u;
    unsigned int nb_single_line_pass_calls_w_change = 0u;
    unsigned int nb_observer_callback_calls = 0u;
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
    using Type = unsigned int;
    using Container = std::vector<Tile::Type>;
    static constexpr Type ROW = 0u, COL = 1u;
public:
    Line(Type type, size_t index, size_t size, Tile::Type init_tile = Tile::UNKNOWN);
    Line(const Line& other, Tile::Type init_tile);
    Line(Type type, size_t index, const Container& tiles);
    Line(Type type, size_t index, Container&& tiles);
public:
    Type get_type() const;
    size_t get_index() const;
    const Container& get_tiles() const;
    Container& get_tiles();
    size_t size() const;
    Tile::Type at(size_t idx) const;
    bool compatible(const Line& other) const;
    bool add(const Line& other);
    void reduce(const Line& other);
private:
    Type type;
    size_t index;
    Container tiles;
};


bool operator==(const Line& lhs, const Line& rhs);
bool operator!=(const Line& lhs, const Line& rhs);

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

    size_t get_width() const { return width; }
    size_t get_height() const { return height; }

    const std::string& get_name() const;
    void set_name(const std::string& name);

    Tile::Type get(size_t x, size_t y) const;

    template <Line::Type type>
    Line get_line(size_t index) const;

    Line get_line(Line::Type type, size_t index) const;

    bool set(size_t x, size_t y, Tile::Type val);
    void reset();

    bool is_solved() const;

private:
    size_t                      width;
    size_t                      height;
    std::string                 name;
    std::vector<Tile::Type>     grid;            // 2D array of tiles
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
    // By default the solver will look for all the solutions of the input grid.
    // The otional argument max_nb_solutions can be used to limit the number of solutions discovered by the algorithm.
    //
    enum class Status { OK, ABORTED, CONTRADICTORY_GRID };
    using Solutions = std::vector<OutputGrid>;
    virtual std::pair<Status, Solutions> solve(const InputGrid& grid_input, unsigned int max_nb_solutions = 0u) const = 0;

    //
    // Set an optional observer on the solver
    //
    // It will be notified of certain events, allowing the user to follow the solving process step by step.
    //
    // The observer is a function object with the following signature:
    //
    // void observer(Event event, const Line* delta, unsigned int depth);
    //
    //  * event = DELTA_LINE        delta != nullptr            depth is set
    //
    //      A line of the output grid has been updated, the delta between the previous value of that line
    //      and the new one is given in delta.
    //      The depth is set to zero initially, then it is the same value as that of the last BRANCHING event.
    //
    //  * event = BRANCHING         delta = nullptr             depth > 0
    //
    //      This event occurs when the algorithm is branching between several alternative solutions, or
    //      when it is going back to an earlier branch. Upon starting a new branch the depth is increased
    //      by one. In the other case, depth is set the the value of the earlier branch.
    //      NB: The user must keep track of the state of the grid at each branching point in order to be
    //          able to reconstruct the final solutions based on the observer events only.
    //      NB: There is no BRANCHING event sent at the beginning of the solving process (depth = 0),
    //          therefore the depth of the first BRANCHING event is always 1.
    //
    //  * event = SOLVED_GRID       delta = nullptr             depth is set
    //
    //      A solution grid has been found. The sum of all the delta lines up until that stage is the solved grid.
    //
    enum class Event { DELTA_LINE, BRANCHING, SOLVED_GRID };
    using Observer = std::function<void(Event,const Line*,unsigned int)>;
    virtual void set_observer(Observer observer) = 0;


    //
    // Set optional stats
    //
    // The stat object is reset at the beginning of each call to Solver::solve()
    //
    virtual void set_stats(GridStats& stats) = 0;


    //
    // Set an abort function
    //
    // If set, the solver will regularly call this function and abort its processing in case it returns true.
    // The solver will return the fully completed solutions it has already computed.
    //
    using Abort = std::function<bool()>;
    virtual void set_abort_function(Abort abort) = 0;
};


std::ostream& operator<<(std::ostream& ostream, Solver::Status status);
std::ostream& operator<<(std::ostream& ostream, Solver::Event event);


/*
 * Validation method: check that the input grid is valid and has a unique solution.
 *
 * Returns a pair composed of an integer code and an error message. The code is as follows:
 *
 *  -1  ERR     The input grid is invalid
 *   0  ZERO    No solution found
 *   1  OK      Valid grid with a unique solution
 *   2  MULT    The solution is not unique
 *
 * NB: The case where the input grid's constraint are not compatible is reported as an ERR (-1): contradictory grid.
 *     An input grid for which the constraints are coherent should have at least one solution. Therefore the validation
 *     code ZERO (0) can only be returned if the solver just gave up finding a solution.
 */
using ValidationCode = int;
std::pair<ValidationCode, std::string> validate_input_grid(const Solver& solver, const InputGrid& grid_input);


std::string validation_code_str(ValidationCode code);


/*
 * Factory for the reference grid solver
 */
std::unique_ptr<Solver> get_ref_solver();


/*
 * Build an InputGrid from an OutputGrid
 */
InputGrid get_input_grid_from(const OutputGrid& grid);

} // namespace picross

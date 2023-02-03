/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - This is the MAIN include file of the library
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include "picross_input_grid.h"
#include "picross_io.h"
#include "picross_stats.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>


namespace picross
{

/*
 * Get the version of the library
 */
std::string_view get_version_string();


/*
 * A tile is the base element to construct lines and grids: it can be empty or filled
 */
enum class Tile : unsigned char
{
    UNKNOWN = 0,
    EMPTY = 1,
    FILLED = 2
};


/*
 * Line class
 *
 *   A line can be either a row or a column of a grid. It consists of an array of tiles.
 */
class Line
{
public:
    using Index = unsigned int;
    using Type = unsigned int;
    static constexpr Type ROW = 0u, COL = 1u;
    using Container = std::vector<Tile>;
public:
    Line(Type type, Index index, std::size_t size, Tile init_tile = Tile::UNKNOWN);
    Line(Type type, Index index, const Container& tiles);
    Line(Type type, Index index, Container&& tiles);

    // copyable & movable
    Line(const Line&) = default;
    Line& operator=(const Line&);
    Line(Line&&) noexcept = default;
    Line& operator=(Line&&) noexcept;
public:
    Type type() const { return m_type; }
    Index index() const { return m_index; }
    std::size_t size() const { return m_tiles.size(); }
    const Tile* tiles() const { return m_tiles.data(); }
    const Tile& operator[](std::size_t idx) const { return m_tiles[idx]; }
    const Tile& at(std::size_t idx) const { return m_tiles.at(idx); }
    const Tile* begin() const { return m_tiles.data(); }
    const Tile* end() const { return m_tiles.data() + m_tiles.size(); }
    bool is_completed() const;

    Tile* tiles() { return m_tiles.data(); }
    Tile& operator[](std::size_t idx) { return m_tiles[idx]; }
    Tile& at(std::size_t idx) { return m_tiles.at(idx); }
    Tile* begin() { return m_tiles.data(); }
    Tile* end() { return m_tiles.data() + m_tiles.size(); }
private:
    const Type      m_type;
    const Index     m_index;
    Container       m_tiles;
};

bool operator==(const Line& lhs, const Line& rhs);
bool operator!=(const Line& lhs, const Line& rhs);
bool are_compatible(const Line& lhs, const Line& rhs);
std::ostream& operator<<(std::ostream& ostream, const Line& line);
std::string str_line_type(Line::Type type);
std::string str_line_full(const Line& line);


/*
 * Line Identifier
 */
struct LineId
{
    LineId(Line::Type type = Line::ROW, Line::Index index = 0u)
        : m_type(type), m_index(index)
    {}

    LineId(const Line& line)
        : m_type(line.type()), m_index(line.index())
    {}

    Line::Type  m_type;
    Line::Index m_index;
};

std::string str_line_id(const LineId& line_id);


/*
 * OutputGrid class
 *
 *   A partially or fully solved Picross grid
 */
class Grid;         // Fwd declaration
class OutputGrid
{
public:
    OutputGrid(std::size_t width, std::size_t height, const std::string& name = std::string{});
    OutputGrid(const Grid&);
    OutputGrid(Grid&&);
    ~OutputGrid();

    OutputGrid(const OutputGrid&);
    OutputGrid(OutputGrid&&) noexcept;
    OutputGrid& operator=(const OutputGrid&);
    OutputGrid& operator=(OutputGrid&&) noexcept;

    std::size_t width() const;
    std::size_t height() const;

    const std::string& name() const;

    Tile get_tile(Line::Index x, Line::Index y) const;

    template <Line::Type Type>
    Line get_line(Line::Index index) const;

    Line get_line(Line::Type type, Line::Index index) const;
    Line get_line(const LineId& line_id) const;

    bool is_completed() const;

    std::size_t hash() const;

    bool set_tile(Line::Index x, Line::Index y, Tile val);
    void reset();

    friend bool operator==(const OutputGrid& lhs, const OutputGrid& rhs);
    friend bool operator!=(const OutputGrid& lhs, const OutputGrid& rhs);

    friend std::ostream& operator<<(std::ostream& ostream, const OutputGrid& grid);
private:
    std::unique_ptr<Grid> p_grid;
};


/*
 * Return the grid size as a string "WxH" with W the width and H the height
 */
std::string str_output_grid_size(const OutputGrid& grid);


/*
 * Grid solver interface
 */
class Solver
{
public:
    virtual ~Solver() = default;

    // Return status of the solver
    enum class Status
    {
        OK,                     // Solver process completed and one or more solutions found
        ABORTED,                // Solver process aborted
        CONTRADICTORY_GRID,     // Not solvable
        NOT_LINE_SOLVABLE       // Not line solvable (i.e. not solvable without a branching algorithm)
    };

    struct Solution
    {
        OutputGrid grid;
        unsigned int branching_depth;
        bool partial;
    };
    using Solutions = std::vector<Solution>;

    // Solver result
    struct Result
    {
        Status status;
        Solutions solutions;
    };

    //
    // Solve a grid
    //
    // By default the solver will look for all the solutions of the input grid.
    // The optional argument max_nb_solutions can be used to limit the number of solutions discovered by the algorithm.
    // max_nb_solutions = 0 means no limit
    //
    // If the solver returns with status Status::NOT_LINE_SOLVABLE, the partially solved grid is part of the
    // solutions
    //
    virtual Result solve(const InputGrid& input_grid, unsigned int max_nb_solutions = 0u) const = 0;


    //
    // Solve a grid
    //
    // The callback solution_found gets called each time the solver finds a solution.
    // If the callback returns true, the solver process continue, otherwise it will stop and this
    // method will return with status Status::ABORTED
    //
    // If the solver returns with status Status::NOT_LINE_SOLVABLE, the partially solved grid is passed to the
    // callback function solution_found, with the partial flag set to true
    //
    using SolutionFound = std::function<bool(Solution&&)>;
    virtual Status solve(const InputGrid& input_grid, SolutionFound solution_found) const = 0;


    //
    // Set an optional observer on the solver
    //
    // It will be notified of certain events, allowing the user to follow the solving process step by step.
    //
    // The observer is a function object with the following signature:
    //
    // void observer(Event event, const Line* line, unsigned int depth, unsigned int misc);
    //
    //  * event = KNOWN_LINE        line = known_line           depth is set        misc = nb_alternatives (before)
    //  * event = DELTA_LINE        line = delta                depth is set        misc = nb_alternatives (after)
    //
    //      A line of the output grid has been updated, the delta between the previous value of that line
    //      and the new one is given in event DELTA_LINE.
    //      The depth is set to zero initially, then it is the same value as that of the last BRANCHING event.
    //      The number of alternatives before (resp. after) the line solver are given in the event KNOWN_LINE
    //      (resp. DELTA_LINE).
    //
    //  * event = BRANCHING         line = known_line           depth >= 0          misc = nb_alternatives    (NODE)
    //  * event = BRANCHING         line = nullptr              depth > 0           misc = 0                  (EDGE)
    //
    //      This event occurs when the algorithm is branching between several alternative solutions, or
    //      when it is going back to an earlier branch. Upon starting a new branch the depth is increased
    //      by one. In the other case, depth is set the the value of the earlier branch.
    //      NB: The user must keep track of the state of the grid at each branching point in order to be
    //          able to reconstruct the final solutions based on the observer events only.
    //      NB: There are two kinds of BRANCHING events: the NODE event which provides the known_tiles of the
    //          branching line and the number of alternatives, and the EDGE event each time an alternative
    //          of the branching line is being tested.
    //
    //  * event = SOLVED_GRID       line = nullptr              depth is set        misc = 0
    //
    //      A solution grid has been found. The sum of all the delta lines up until that stage is the solved grid.
    //
    //  * event = INTERNAL_STATE    line = nullptr              depth is set        misc = state
    //
    enum class Event { KNOWN_LINE, DELTA_LINE, BRANCHING, SOLVED_GRID, INTERNAL_STATE };
    using Observer = std::function<void(Event,const Line*,unsigned int,unsigned int)>;
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
 * Factory for the reference grid solver
 */
std::unique_ptr<Solver> get_ref_solver();


/*
 * Factory for a line solver
 */
std::unique_ptr<Solver> get_line_solver();


/*
* Validation code:
*
*  -1  ERR     The input grid is invalid
*   0  ZERO    No solution found
*   1  OK      Valid grid with a unique solution
*   2  MULT    The solution is not unique
*/
using ValidationCode = int;
std::string_view str_validation_code(ValidationCode code);


/*
 * Validation method: check that the input grid is valid and has a unique solution.
 *
 * Return the validation code (ERR, ZERO, OK, MULT), the branching depth required to solve it (which determines
 * if the grid is line solvable or not), and an optional message regarding the grid validation process.
 *
 * NB: The case where the input grid's constraint are not compatible is reported as an ERR (-1): contradictory grid.
 *     An input grid for which the constraints are coherent should have at least one solution. Therefore the validation
 *     code ZERO (0) can only be returned if the solver just gave up finding a solution.
 */
struct ValidationResult
{
    ValidationCode code;
    unsigned int branching_depth;
    std::string msg;
};
ValidationResult validate_input_grid(const Solver& solver, const InputGrid& input_grid);


/*
 * Functions to test an output grid against a set of input contraints
 *
 * - is_solution: returns true if the output grid is a solution of the set of constraints
 * - list_incompatible_lines: returns the list of all incompatible constraints
 *
 * NB: Will throw on an invalid InputGrid (i.e. not passing check_input_grid)
 * NB: Will throw if the input and output grids' size do not match
 * NB: list_incompatible_lines will throw on a partial output (one with Tile::UNKNWON tiles)
 */
bool is_solution(const InputGrid& input_grid, const OutputGrid& output_grid);
std::vector<LineId> list_incompatible_lines(const InputGrid& input_grid, const OutputGrid& output_grid);


/*
 * Utility functions to build an input constraint from a fully defined Line,
 * or an InputGrid from a fully defined OutputGrid (no Tile::UNKNOWN)
 */
InputGrid::Constraint get_constraint_from(const Line& line);
InputGrid get_input_grid_from(const OutputGrid& grid);

} // namespace picross

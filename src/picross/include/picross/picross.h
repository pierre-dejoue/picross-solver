/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#pragma once


#include <memory>
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
    using Type = int;
    constexpr Type UNKNOWN = -1, ZERO = 0, ONE = 1;
}


/*
 * InputConstraint class
 *
 *   It defines the constraints that apply to a single line (row or column) of a Picross grid.
 *   It provides the size of each of the groups of contiguous filled tiles.
 */
using InputConstraint = std::vector<unsigned int>;


/*
 * InputGrid class
 *
 *   A data structure to store the information related to an unsolved Picross grid.
 *   This is basically the input information of the solver.
 */
struct InputGrid
{
    std::string name;
    std::vector<InputConstraint> rows;
    std::vector<InputConstraint> columns;
};


std::pair<bool, std::string> check_grid_input(const InputGrid& grid_input);


/*
 * GridStats struct
 *
 *   A data structure to store some stat related to the solver (optional).
 */
struct GridStats
{
    unsigned int guess_max_nested_level = 0u;
    unsigned int guess_total_calls = 0u;
    unsigned int guess_max_alternatives = 0u;
    unsigned int guess_total_alternatives = 0u;
    unsigned int nb_reduce_lines_calls = 0u;
    unsigned int max_reduce_list_size = 0u;
    unsigned int max_theoretical_nb_alternatives = 0u;
    unsigned int total_lines_reduced = 0u;
    unsigned int nb_add_and_filter_calls = 0u;
    unsigned int max_add_and_filter_list_size = 0u;
    unsigned int total_lines_added_and_filtered = 0u;
    unsigned int nb_reduce_all_rows_and_colums_calls = 0u;
};


void print_grid_stats(const GridStats* stats, std::ostream& ostream);


/*
 * SolvedGrid class
 *
 *   An interface to view the solution of a Picross grid
 */
class SolvedGrid
{
public:
    virtual const std::string& get_name() const = 0;
    virtual unsigned int get_height() const = 0;
    virtual unsigned int get_width() const = 0;
    virtual std::vector<Tile::Type> get_row(unsigned int index) const = 0;
    virtual void print(std::ostream& ostream) const = 0;
};


/*
 * Grid solver interface
 */
class Solver
{
public:
    virtual std::vector<std::unique_ptr<SolvedGrid>> solve(const InputGrid& grid_input, GridStats* stats = nullptr) const = 0;
};


/*
 * Factory for the reference grid solver
 */
std::unique_ptr<Solver> getRefSolver();

} // namespace picross

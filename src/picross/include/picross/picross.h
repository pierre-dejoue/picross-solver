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
    unsigned int nb_reduce_lines_calls = 0u;
    unsigned int max_reduce_list_size = 0u;
    unsigned int max_theoretical_nb_alternatives = 0u;
    unsigned int total_lines_reduced = 0u;
    unsigned int nb_add_and_filter_calls = 0u;
    unsigned int max_add_and_filter_list_size = 0u;
    unsigned int total_lines_added_and_filtered = 0u;
    unsigned int nb_reduce_all_lines_calls = 0u;
};


void print_grid_stats(const GridStats* stats, std::ostream& ostream);


/*
 * OutputGrid class
 *
 *   A partially of fully solved Picross grid
 */
class OutputGrid
{
public:
    OutputGrid(size_t width, size_t height, const std::string& name = "");

    const std::string& get_name() const;
    size_t get_width() const;
    size_t get_height() const;

    Tile::Type get(size_t x, size_t y) const;
    void set(size_t x, size_t y, Tile::Type t);

    bool is_solved() const;
    const Tile::Type* data() const;
    virtual void print(std::ostream& ostream) const;

private:
    const size_t                                width, height;
    const std::string                           name;
    std::vector<Tile::Type>                     grid;            // 2D array of tiles
};

/*
 * Grid solver interface
 */
class Solver
{
public:
    using Solutions = std::vector<OutputGrid>;

    virtual ~Solver() = default;
    virtual Solutions solve(const InputGrid& grid_input, GridStats* stats = nullptr) const = 0;
};


/*
 * Factory for the reference grid solver
 */
std::unique_ptr<Solver> get_ref_solver();

} // namespace picross

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
bool is_fully_defined(const Line& line);
std::string str_line(const Line& line);
std::string str_line_type(Line::Type type);
Line line_delta(const Line& line1, const Line& line2);


/*
 * Utility class used to store already computed binomial number.
 *
 * This is to optimize the computation of the number of alternatives on empty lines.
 */
class BinomialCoefficientsCache
{
public:
    using Rep = unsigned int;

    BinomialCoefficientsCache() = default;
    BinomialCoefficientsCache(const BinomialCoefficientsCache& other) = delete;
    BinomialCoefficientsCache& operator==(const BinomialCoefficientsCache& other) = delete;

    static Rep overflowValue();

    /*
     * Returns the number of ways to divide nb_elts into nb_buckets.
     *
     * Equal to the binomial coefficient (n k) with n = nb_elts + nb_buckets - 1  and k = nb_buckets - 1
     *
     * Returns std::numeric_limits<unsigned int>::max() in case of overflow (which happens rapidly)
     */
    Rep nb_alternatives_for_fixed_nb_of_partitions(unsigned int nb_cells, unsigned int nb_partitions);
private:
    std::vector<Rep> binomial_numbers;
};


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
    std::pair<bool, unsigned int> line_trivial_reduction(Line& line, BinomialCoefficientsCache& binomial) const;
    std::vector<Line> build_all_possible_lines(const Line& known_tiles) const;
    std::pair<Line, unsigned int> reduce_and_count_alternatives(const Line& filter_line, GridStats * stats) const;
    bool compatible(const Line& line) const;
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
    bool solve(unsigned int max_nb_solutions = 0u);
private:
    bool all_lines_completed() const;
    bool set_line(const Line& line);
    bool single_line_initial_pass(Line::Type type, unsigned int index);
    bool single_line_pass(Line::Type type, unsigned int index);
    bool full_side_pass(Line::Type type, bool first_pass = false);
    bool full_grid_pass(bool first_pass = false);
    bool set_w_reduce_flag(size_t x, size_t y, Tile::Type t);
    bool guess(unsigned int max_nb_solutions) const;
    bool valid_solution() const;
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
    std::unique_ptr<BinomialCoefficientsCache>  binomial;
};


/*
 * Grid solver: an implementation
 */
class RefSolver : public Solver
{
public:
    Solver::Solutions solve(const InputGrid& grid_input, unsigned int max_nb_solutions) const override;
    void set_observer(Observer observer) override;
    void set_stats(GridStats& stats) override;
private:
    Observer observer;
    GridStats* stats;
};

} // namespace picross

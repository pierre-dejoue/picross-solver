/*******************************************************************************
 * PICROSS SOLVER
 *
 *   This file implements the core functions of the Picross solver
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#include "picross_solver.h"


#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <exception>
#include <functional>
#include <list>
#include <memory>
#include <numeric>
#include <ostream>
#include <sstream>
#include <utility>
#include <string>
#include <vector>


namespace picross
{
namespace
{
    class PicrossLineAdditionError : public std::runtime_error
    {
    public:
        PicrossLineAdditionError() : std::runtime_error("[PicrossSolver] An error was detected adding two lines")
        {}
    };


    class PicrossGridCannotBeSolved : public std::runtime_error
    {
    public:
        PicrossGridCannotBeSolved() : std::runtime_error("[PicrossSolver] The current grid cannot be solved")
        {}
    };


    unsigned int nb_alternatives_for_fixed_nb_of_partitions(unsigned int nb_cells, unsigned int nb_partitions)
    {
        if (nb_cells == 0u || nb_partitions <= 1u)
        {
            return 1u;
        }
        else
        {
            unsigned int accumulator = 0;
            for (unsigned int n = 0u; n <= nb_cells; n++) { accumulator += nb_alternatives_for_fixed_nb_of_partitions(nb_cells - n, nb_partitions - 1u); }
            return accumulator;
        }
    }
} // Anonymous namespace


namespace Tile
{
    char str(Type t)
    {
        if(t == UNKNOWN) { return '?'; }
        if(t == ZERO)    { return ' '; }
        if(t == ONE)     { return '#'; }
        std::ostringstream oss;
        oss << "Invalid tile value: " << t;
        throw std::invalid_argument(oss.str());
    }

    Type add(Type t1, Type t2)
    {
        if(t1 == t2 || t2 == Tile::UNKNOWN) { return t1; }
        else if(t1 == Tile::UNKNOWN)        { return t2; }
        else                                { throw PicrossLineAdditionError(); }
    }

    Type reduce(Type t1, Type t2)
    {
        if(t1 == t2) { return t1; }
        else         { return Tile::UNKNOWN; }
    }
} // end namespace Tile


Line::Line(Line::Type type, const std::vector<Tile::Type>& vect) :
    type(type),
    tiles(vect)
{
}


const std::vector<Tile::Type>& Line::get_tiles() const
{
    return tiles;
}


std::vector<Tile::Type>::const_iterator Line::cbegin() const
{
    return tiles.cbegin();
}


std::vector<Tile::Type>::const_iterator Line::cend() const
{
    return tiles.cend();
}


/* Line::add combines the information of two lines into a single one. If the data does not
 * match (a tile is empty in one line and filled in in the other), then an exception is raised.
 *
 * Example:
 *         line1:    ....##??????
 *         line2:    ..????##..??
 * line1 + line2:    ....####..??
 */
void Line::add(const Line& line)
{
    if(line.type != type)                 { throw std::invalid_argument("Add: Line type mismatch"); }
    if(line.tiles.size() != tiles.size()) { throw std::invalid_argument("Add: Line size mismatch"); }
    std::transform(tiles.begin(), tiles.end(), line.tiles.begin(), tiles.begin(), Tile::add);
}


/* Line::reduce captures the information that is common to two lines.
 * To the contrary of Line::add it does not throw an exception.
 *
 * Example:
 *         line1:    ??..######..
 *         line2:    ??....######
 * line1 ^ line2:    ??..??####??
 */
void Line::reduce(const Line& line)
{
    if(line.type != type)                 { throw std::invalid_argument("Reduce: Line type mismatch"); }
    if(line.tiles.size() != tiles.size()) { throw std::invalid_argument("Reduce: Line size mismatch"); }
    std::transform(tiles.begin(), tiles.end(), line.tiles.begin(), tiles.begin(), Tile::reduce);
}


/* Add (with Tile::add) a filter line to each line in a list. If the addition fails,
 * the offending line is discarded from the list
 */
void add_and_filter_lines(std::list<Line>& list_of_lines, const Line& filter_line, GridStats* stats)
{
    // Stats
    if(stats != nullptr)
    {
         const auto list_size = static_cast<unsigned int>(list_of_lines.size());
         stats->nb_add_and_filter_calls++;
         stats->total_lines_added_and_filtered += list_size;
         if(list_size > stats->max_add_and_filter_list_size) { stats->max_add_and_filter_list_size = list_size; }
    }

    std::list<Line>::iterator line = list_of_lines.begin();
    while(line != list_of_lines.end())
    {
        try
        {
            line->add(filter_line);
            line++;
        }
        catch(PicrossLineAdditionError&)
        {
            line = list_of_lines.erase(line);
        }
    }
}


/* Reduce a list of lines and return one line that comprises the information common to all */
Line reduce_lines(const std::list<Line>& list_of_lines, GridStats * stats)
{
    // Stats
    if(stats != nullptr)
    {
         const auto list_size = static_cast<unsigned int>(list_of_lines.size());
         stats->nb_reduce_lines_calls++;
         stats->total_lines_reduced += list_size;
         if(list_size > stats->max_reduce_list_size) { stats->max_reduce_list_size = list_size; }
    }

    if(list_of_lines.empty()) { throw std::invalid_argument("Cannot reduce an empty list of lines"); }
    Line new_line = list_of_lines.front();
    for(std::list<Line>::const_iterator line = ++list_of_lines.begin(); line != list_of_lines.end(); line++)
    {
        new_line.reduce(*line);
    }
    return new_line;
}


bool Line::is_all_one_color(Tile::Type color) const
{
    for(size_t idx = 0; idx < tiles.size(); idx++)
    {
        if(tiles.at(idx) != color) { return false; }
    }
    return true;
}


void Line::print(std::ostream& ostream) const
{
    if(type == Line::ROW)    { ostream << "ROW    "; }
    if(type == Line::COLUMN) { ostream << "COLUMN "; }
    ostream << str_line(*this) << std::endl;
}


std::string str_line(const Line& line)
{
    std::string out_str = "";
    for(int idx = 0; idx < line.get_size(); idx++) { out_str += Tile::str(line.get_tile(idx)); }
    return out_str;
}


std::string str_line_type(Line::Type type)
{
    if (type == Line::ROW) { return "ROW"; }
    if (type == Line::COLUMN) { return "COLUMN"; }
    assert(0);
    return "UNKNOWN";
}

namespace
{
    std::vector<Constraint> row_constraints_from_input_grid(const InputGrid& grid)
    {
        std::vector<Constraint> rows;
        rows.reserve(grid.rows.size());
        std::transform(grid.rows.cbegin(), grid.rows.cend(), std::back_inserter(rows), [](const InputConstraint& c) { return Constraint(Line::ROW, c); });
        return rows;
    }

    std::vector<Constraint> column_constraints_from_input_grid(const InputGrid& grid)
    {
        std::vector<Constraint> columns;
        columns.reserve(grid.columns.size());
        std::transform(grid.columns.cbegin(), grid.columns.cend(), std::back_inserter(columns), [](const InputConstraint& c) { return Constraint(Line::COLUMN, c); });
        return columns;
    }
}


Grid::Grid(const InputGrid& grid, std::vector<std::unique_ptr<SolvedGrid>>& solutions, GridStats* stats) :
    Grid(grid.name, row_constraints_from_input_grid(grid), column_constraints_from_input_grid(grid), solutions, stats)
{
}


/* Copy constructor. Do a full copy */
Grid::Grid(const Grid& other) :
    saved_solutions(other.saved_solutions)
{
    *this = other;
}


/* Assignment operator. Do a full copy. */
Grid& Grid::operator=(const Grid& other)
{
    height = other.height;
    width = other.width;
    grid_name = other.grid_name;
    rows = other.rows;
    columns = other.columns;
    // Skip reference saved_solutions
    stats = other.stats;
    nested_level = other.nested_level;

    // Allocate memory
    grid = new Tile::Type*[width];
    for(unsigned int x = 0u; x < width; x++)
    {
        grid[x] = new Tile::Type[height];
    }

    // Copy the grid contents from this grid to the new one
    for(unsigned int x = 0u; x < width; x++)
    {
        for(unsigned int y = 0u; y < height; y++)
        {
            grid[x][y] = other.grid[x][y];
        }
    }

    // Ignore the other members (TODO rework)

    return *this;
}


Grid::~Grid()
{
    // Free memory
    for(unsigned int x = 0u; x < width; x++)
    {
       delete[] grid[x];
    }
    delete[] grid;
    grid = nullptr;
}


Grid::Grid(const std::string& grid_name, const std::vector<Constraint>& rows, const std::vector<Constraint>& columns, std::vector<std::unique_ptr<SolvedGrid>>& solutions, GridStats* stats, unsigned int nested_level) :
    height(static_cast<unsigned int>(rows.size())),
    width(static_cast<unsigned int>(columns.size())),
    grid_name(grid_name),
    rows(rows),
    columns(columns),
    saved_solutions(solutions),
    grid(nullptr),
    stats(stats),
    nested_level(nested_level)
{
    // Allocate memory
    grid = new Tile::Type*[width];
    for (unsigned int x = 0u; x < width; x++)
    {
        grid[x] = new Tile::Type[height];
        for (unsigned int y = 0u; y < height; y++)
        {
            grid[x][y] = Tile::UNKNOWN;
        }
    }

    // Other initializations
    line_complete[Line::ROW].resize(height, false);
    line_complete[Line::COLUMN].resize(width, false);
    line_to_be_reduced[Line::ROW].resize(height, true);
    line_to_be_reduced[Line::COLUMN].resize(width, true);
    alternatives[Line::ROW].resize(height);
    alternatives[Line::COLUMN].resize(width);

    // Stats TODO move
    if (stats != nullptr && nested_level > stats->guess_max_nested_level) { stats->guess_max_nested_level = nested_level; }
}


Grid::Grid(const Grid& parent, unsigned int nested_level) :
    Grid(parent.grid_name, parent.rows, parent.columns, parent.saved_solutions, parent.stats, nested_level)
{
}


Line Grid::get_line(Line::Type type, unsigned int index) const
{
    if(type == Line::ROW)
    {
        if(index >= height) { throw std::invalid_argument("Grid::get_line: row index is out of range"); }
        std::vector<Tile::Type> vect;
        for(unsigned int x = 0u; x < width; x++) { vect.push_back(grid[x][index]); }
        return Line(type, vect);
    }
    else if(type == Line::COLUMN)
    {
        if (index >= width) { throw std::invalid_argument("Grid::get_line: column index is out of range"); }
        std::vector<Tile::Type> vect(grid[index], grid[index] + height);
        return Line(type, vect);
    }
    else { throw std::invalid_argument("Grid::get_line: wrong type argument"); }
}


bool Grid::set(unsigned int x, unsigned int y, Tile::Type t)
{
    if(x >= width) { throw std::invalid_argument("Grid::set: x is out of range"); }
    if(y >= height) { throw std::invalid_argument("Grid::set: y is out of range"); }
    if(t != grid[x][y])
    {
        // modify grid
        grid[x][y] = t;

        // marked the impacted row and column with flag "to be reduced".
        line_to_be_reduced[Line::ROW][y] = true;
        line_to_be_reduced[Line::COLUMN][x] = true;

        // return true since the grid was modifed.
        return true;
    }
    else
    {
        return false;
    }
}


bool Grid::set_line(Line line, unsigned int index)
{
    bool changed = false;
    if(line.get_type() == Line::ROW && line.get_size() == width)
    {
        for(unsigned int x = 0u; x < width; x++) { changed |= set(x, index, line.get_tile(x)); }
    }
    else if(line.get_type() == Line::COLUMN && line.get_size() == height)
    {
        for(unsigned int y = 0u; y < height; y++) { changed |= set(index, y, line.get_tile(y)); }
    }
    else
    {
        throw std::invalid_argument("Grid::set_line: wrong arguments");
    }
    return changed;
}


const std::string& Grid::get_name() const
{
    return grid_name;
}


unsigned int Grid::get_height() const
{
    return height;
}


unsigned int Grid::get_width() const
{
    return width;
}


std::vector<Tile::Type> Grid::get_row(unsigned int index) const
{
    assert(is_solved());
    return get_line(Line::ROW, index).get_tiles();
}


void Grid::print(std::ostream& ostream) const
{
    assert(is_solved());
    ostream << "Grid " << width << "x" << height << ":" << std::endl;
    for(unsigned int y = 0u; y < height; y++)
    {
        Line line = get_line(Line::ROW, y);
        ostream << "  " << str_line(line) << std::endl;
    }
    ostream << std::endl;
}


bool Grid::reduce_one_line(Line::Type type, unsigned int index)
{
    Line filter_line = get_line(type, index);
    const Constraint * line_constraint;
    unsigned int line_size;

    if(type == Line::ROW)
    {
        line_constraint = &(rows.at(index));
        line_size = width;
    }
    else if(type == Line::COLUMN)
    {
        line_constraint = &(columns.at(index));
        line_size = height;
    }

    // OPTIM: if the color of every tile in the line is unknown, there is a simple way to determine
    // if the reduce operation is relevant: max block size must be greater than line_size - min_line_size.
    if(filter_line.is_all_one_color(Tile::UNKNOWN))
    {
        if(line_constraint->max_block_size() != 0 &&
           line_constraint->max_block_size() <= (line_size - line_constraint->get_min_line_size()))
        {
            // Need not reduce this line until one of the tiles is modified.
            line_to_be_reduced[type][index] = false;

            // We shall compute the number of alternatives anyway (in case we have to make an hypothesis on the grid)
            alternatives[type].at(index) = line_constraint->theoretical_nb_alternatives(line_size, stats);

            // No change made on this line
            return false;
        }
    }
    // End of OPTIM.

    // Compute all possible lines that match the data already present in the grid and the line constraints
    std::list<Line> all_lines = line_constraint->build_all_possible_lines_with_size(line_size, filter_line, stats);
    alternatives[type].at(index) = static_cast<unsigned int>(all_lines.size());

    // If the list of all lines is empty, it means the grid data is contradictory. Throw an exception.
    if(alternatives[type].at(index) == 0) { throw PicrossGridCannotBeSolved(); }

    // If the list comprises of only one element, it means we solved that line
    if(alternatives[type].at(index) == 1) { line_complete[type].at(index) = true; }

    // In any case, update the grid data with the reduced line resulting from list all_lines
    Line new_line = reduce_lines(all_lines, stats);
    bool changed = set_line(new_line, index);

    // This line does not need to be reduced until one of the tiles is modified.
    line_to_be_reduced[type][index] = false;

    // return true if some tile was modified. False otherwise.
    return changed;
}


bool Grid::reduce_all_rows_and_columns()
{
    // Reduce all columns and all rows. Return false if no change was made on the grid.
    bool changed = false;
    for(unsigned int x = 0u; x < width; x++)
    {
        if(!line_complete[Line::COLUMN][x] && line_to_be_reduced[Line::COLUMN][x]) { changed |= reduce_one_line(Line::COLUMN, x); }
    }
    for(unsigned int y = 0u; y < height; y++)
    {
        if(!line_complete[Line::ROW][y] && line_to_be_reduced[Line::ROW][y])       { changed |= reduce_one_line(Line::ROW, y); }
    }

    // Stats
    if(stats != nullptr) { stats->nb_reduce_all_rows_and_colums_calls++; }

    return changed;
}


bool Grid::is_solved() const
{
    bool on_rows = std::accumulate(line_complete[Line::ROW].cbegin(), line_complete[Line::ROW].cend(), true, std::logical_and<bool>());
    bool on_columns = std::accumulate(line_complete[Line::COLUMN].cbegin(), line_complete[Line::COLUMN].cend(), true, std::logical_and<bool>());
    return on_rows || on_columns;
}


// Utility class used to find the min value, greater ot equal to 2. Used in Grid::solve()
class f_min_greater_than_2 {
public:
    f_min_greater_than_2(unsigned int& min) : min(min) {}

    void operator()(unsigned int val)
    {
        if(val >= 2u)
        {
            if(min < 2u || val < min)
            {
                min = val;
            }
        }
    }
private:
    unsigned int& min;
};


bool Grid::solve()
{
    try
    {
        while(reduce_all_rows_and_columns())
        {
            // While the reduce method is successful, use it to find the filled and empty tiles.
        }

        // Are we done?
        if(is_solved())
        {
            save_solution();
            return true;
        }
        // If we are not, we have to make an hypothesis and continue based on that
        else
        {
            // Find the row or column not yet solved with the minimal alternative lines.
            // That is the min of all alternatives greater or equal to 2.
            const Constraint* line_constraint;
            int line_size;
            unsigned int min_alt = 0u;

            for_each(alternatives[Line::ROW].begin(),    alternatives[Line::ROW].end(),    f_min_greater_than_2(min_alt));
            for_each(alternatives[Line::COLUMN].begin(), alternatives[Line::COLUMN].end(), f_min_greater_than_2(min_alt));
            if(min_alt == 0u) { throw std::logic_error("Grid::solve: no min value, this should not happen!"); }

            // Some stats
            if(stats != nullptr && min_alt > stats->guess_max_alternatives)  { stats->guess_max_alternatives = min_alt; }
            if(stats != nullptr)                                             { stats->guess_total_alternatives += min_alt; }

            // Select one row or one column with the minimal number of alternatives.
            auto alternative_it = std::find_if(alternatives[Line::ROW].cbegin(), alternatives[Line::ROW].cend(), [min_alt](unsigned int alt) { return alt == min_alt; });
            if (alternative_it == alternatives[Line::ROW].end())
            {
                alternative_it = std::find_if(alternatives[Line::COLUMN].cbegin(), alternatives[Line::COLUMN].cend(), [min_alt](unsigned int alt) { return alt == min_alt; });
                if (alternative_it == alternatives[Line::COLUMN].end())
                {
                    // Again, this should not happen.
                    throw std::logic_error("Grid::solve: alternative_it = alternatives[Line::COLUMN].end(). Impossible!");
                }
                guess_line_type = Line::COLUMN;
                guess_line_index = static_cast<unsigned int>(alternative_it - alternatives[Line::COLUMN].cbegin());
                line_constraint = &(columns.at(guess_line_index));
                line_size = height;
            }
            else
            {
                guess_line_type = Line::ROW;
                guess_line_index = static_cast<unsigned int>(alternative_it - alternatives[Line::ROW].cbegin());
                line_constraint = &(rows.at(guess_line_index));
                line_size = width;
            }

            guess_list_of_all_alternatives = line_constraint->build_all_possible_lines_with_size(line_size, get_line(guess_line_type, guess_line_index), stats);

            // Guess!
            return guess();
        }
    }
    catch(PicrossGridCannotBeSolved&)
    {
        // The puzzle couldn't be solved
        return false;
    }
}


bool Grid::guess()
{
    /* This function will test a range of alternatives for one particular line of the grid, each time
     * creating a new instance of the grid class on which the function Grid::solve() is called.
     */
    bool flag_solution_found = false;
    if(stats != nullptr) { stats->guess_total_calls++; }
    for(std::list<Line>::const_iterator guess_line = guess_list_of_all_alternatives.begin(); guess_line != guess_list_of_all_alternatives.end(); guess_line++)
    {
        // Allocate a new grid.
        // One reason to not using the copy contructor: that would copy the guess_list_of_all_alternatives, which is not useful.
        Grid new_grid(*this, nested_level + 1);

        // Copy the grid contents from this grid to the new one
        for(unsigned int x = 0u; x < width; x++)
        {
            for(unsigned int y = 0u; y < height; y++)
            {
                new_grid.grid[x][y] = grid[x][y];
            }
        }

        // Copy also the 'line_complete' status array, so that the new instance of the grid does not lose time on lines that are already solved.
        new_grid.line_complete[Line::ROW]    = line_complete[Line::ROW];
        new_grid.line_complete[Line::COLUMN] = line_complete[Line::COLUMN];

        // Copy also the 'line_to_be_reduced' status array, for the same reason
        new_grid.line_to_be_reduced[Line::ROW]    = line_to_be_reduced[Line::ROW];
        new_grid.line_to_be_reduced[Line::COLUMN] = line_to_be_reduced[Line::COLUMN];

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        if(!new_grid.set_line(*guess_line, guess_line_index)) { throw std::logic_error("Grid::guess: no change in the new grid, will cause infinite loop."); }
        new_grid.line_complete[guess_line->get_type()].at(guess_line_index) = true;

        // Solve the new grid!
        flag_solution_found |= new_grid.solve();
    }
    return flag_solution_found;
}


void Grid::save_solution()
{
    // The copy operator will copy the grid data
    saved_solutions.push_back(std::make_unique<Grid>(*this));
}


Constraint::Constraint(Line::Type type, const InputConstraint& vect) :
    type(type),
    sets_of_ones(vect)
{
    if (sets_of_ones.size() == 0u)
    {
        min_line_size = 0u;
    }
    else
    {
        /* Include at least one zero between the sets of one */
        min_line_size = accumulate(sets_of_ones.cbegin(), sets_of_ones.cend(), 0u) + static_cast<unsigned int>(sets_of_ones.size()) - 1u;
    }
}


unsigned int Constraint::nb_filled_tiles() const
{
    return accumulate(sets_of_ones.cbegin(), sets_of_ones.cend(), 0u);
}


unsigned int Constraint::max_block_size() const
{
    // Sets_of_ones vector must not be empty.
    if(sets_of_ones.empty()) { return 0u; }
    // Compute max
    return *max_element(sets_of_ones.cbegin(), sets_of_ones.cend());
}


void Constraint::print(std::ostream& ostream) const
{
    ostream << "Constraint on a " << str_line_type(type) << ": [ ";
    for(auto& it = sets_of_ones.begin(); it != sets_of_ones.end(); ++it)
    {
        ostream << *it << " ";
    }
    ostream << "]; min_line_size = " << min_line_size << std::endl;
}


int Constraint::theoretical_nb_alternatives(unsigned int line_size, GridStats * stats) const
{
    // Compute the number of alternatives for a line of size 'size', assuming all tiles are Tile::UNKONWN.
    // Number of zeros to add to the minimal size line.
    if(line_size < min_line_size)  { throw std::logic_error("Constraint::theoretical_nb_alternatives: line_size < min_line_size"); }
    unsigned int nb_zeros = line_size - min_line_size;

    unsigned int nb_alternatives = nb_alternatives_for_fixed_nb_of_partitions(nb_zeros, static_cast<unsigned int>(sets_of_ones.size()) + 1);
    if(stats != nullptr && nb_alternatives > stats->max_theoretical_nb_alternatives) {  stats->max_theoretical_nb_alternatives = nb_alternatives; }

    return nb_alternatives;
}


std::list<Line> Constraint::build_all_possible_lines_with_size(unsigned int line_size, const Line& filter_line, GridStats * stats) const
{
    if(filter_line.get_size() != line_size)   { throw std::invalid_argument("Wrong filter line size"); }
    if(filter_line.get_type() != type)   { throw std::invalid_argument("Wrong filter line type"); }

    // Number of zeros to add to the minimal size line.
    if (line_size < min_line_size) { throw std::logic_error("Constraint::build_all_possible_lines_with_size: line_size < min_line_size"); }
    unsigned int nb_zeros = line_size - min_line_size;

    std::list<Line> return_list;
    std::vector<Tile::Type> new_tile_vect(line_size, Tile::ZERO);

    if(sets_of_ones.size() == 0)
    {
        // Return a list with only one all-zero line
        return_list.push_back( Line(type, new_tile_vect) );

        // Filter the return_list against filter_line
        add_and_filter_lines(return_list, filter_line, stats);
    }
    else if(sets_of_ones.size() == 1)
    {
        // Build the list of all possible lines with only one block of continuous filled tiles.
        // NB: in this case nb_zeros = size - sets_of_ones[0]
        for(unsigned int n = 0u; n <= nb_zeros; n++)
        {
            for(unsigned int idx = 0u; idx < n; idx++)                          { new_tile_vect.at(idx)   = Tile::ZERO; }
            for(unsigned int idx = 0u; idx < sets_of_ones[0]; idx++)            { new_tile_vect.at(n+idx) = Tile::ONE;  }
            for(unsigned int idx = n + sets_of_ones[0]; idx < line_size; idx++) { new_tile_vect.at(idx)   = Tile::ZERO; }
            return_list.push_back( Line(type, new_tile_vect) );
        }

        // Filter the return_list against filter_line
        if(!filter_line.is_all_one_color(Tile::UNKNOWN)) { add_and_filter_lines(return_list, filter_line, stats); }
    }
    else
    {
        // For loop on the number of zeros before the first block of ones
        for(unsigned int n = 0u; n <= nb_zeros; n++)
        {
            for(unsigned int idx = 0u; idx < n; idx++)                  { new_tile_vect.at(idx)   = Tile::ZERO; }
            for(unsigned int idx = 0u; idx < sets_of_ones[0]; idx++)    { new_tile_vect.at(n+idx) = Tile::ONE;  }
            const unsigned int begin_size = n + sets_of_ones[0] + 1u;
            new_tile_vect.at(begin_size - 1u) = Tile::ZERO;

            try
            {
                // Add the start of the line (first block of ones) and the beginning of the filter line
                std::vector<Tile::Type> begin_vect(new_tile_vect.begin(), new_tile_vect.begin() + begin_size);
                std::vector<Tile::Type> begin_filter_vect(filter_line.cbegin(), filter_line.cbegin() + begin_size);

                Line begin_line(type, begin_vect);
                Line begin_filter(type, begin_filter_vect);
                begin_line.add(begin_filter);               // can throw PicrossLineAdditionError

                // If OK, then go on and recursively call this function to construct the remaining part of the line.
                std::vector<unsigned int> trim_sets_of_ones(sets_of_ones.begin()+1, sets_of_ones.end());
                Constraint recursive_constraint(type, trim_sets_of_ones);

                std::vector<Tile::Type> end_filter_vect(filter_line.cbegin() + begin_size, filter_line.cbegin() + filter_line.get_size());
                Line end_filter(type, end_filter_vect);

                std::list<Line> recursive_list = recursive_constraint.build_all_possible_lines_with_size(line_size - begin_size, end_filter, stats);

                // Finally, construct the return_list based on the contents of the recursive_list.
                for(std::list<Line>::iterator line = recursive_list.begin(); line != recursive_list.end(); line++)
                {
                    copy(line->cbegin(),  line->cbegin() + line->get_size(), new_tile_vect.begin() + begin_size);
                    return_list.push_back( Line(type, new_tile_vect) );
                }
            }
            catch(PicrossLineAdditionError&)
            {
                // The beginning of the line does not match the filter_line. Do nothing.
            }
        }
        // Filtering is already done, no need to call add_and_filter_lines()..
    }

    return return_list;
}


std::pair<bool, std::string> check_grid_input(const InputGrid& grid_input)
{
    // Sanity check of the grid input
    //  -> height != 0 and width != 0
    //  -> same number of painted cells on rows and columns
    //  -> for each constraint min_line_size is smaller than the width or height of the row or column, respectively.

    const auto width  = static_cast<unsigned int>(grid_input.columns.size());
    const auto height = static_cast<unsigned int>(grid_input.rows.size());

    if(height == 0u)
    {
        std::ostringstream oss;
        oss << "Invalid height = " << height << std::endl;
        return std::make_pair(false, oss.str());
    }
    if (width == 0u)
    {
        std::ostringstream oss;
        oss << "Invalid width = " << width << std::endl;
        return std::make_pair(false, oss.str());
    }
    unsigned int nb_tiles_on_rows = 0u;
    for(auto& c = grid_input.rows.begin(); c != grid_input.rows.end(); c++)
    {
        Constraint row(Line::ROW, *c);
        nb_tiles_on_rows += row.nb_filled_tiles();
        if(row.get_min_line_size() > width)
        {
            std::ostringstream oss;
            oss << "Width = " << width << " of the grid is too small for constraint: ";
            row.print(oss);
            return std::make_pair(false, oss.str());
        }
    }
    unsigned int nb_tiles_on_columns = 0u;
    for(auto& c = grid_input.columns.begin(); c != grid_input.columns.end(); c++)
    {
        Constraint column(Line::COLUMN, *c);
        nb_tiles_on_columns += column.nb_filled_tiles();
        if(column.get_min_line_size() > height)
        {
            std::ostringstream oss;
            oss << "Height = " << height << " of the grid is too small for constraint: ";
            column.print(oss);
            return std::make_pair(false, oss.str());
        }
    }
    if(nb_tiles_on_rows != nb_tiles_on_columns)
    {
        std::ostringstream oss;
        oss << "Number of filled tiles on rows (" << nb_tiles_on_rows << ") and columns (" <<  nb_tiles_on_columns << ") do not match." << std::endl;
        return std::make_pair(false, oss.str());
    }
    return std::make_pair(true, std::string());
}


void print_grid_stats(const GridStats* stats, std::ostream& ostream)
{
    if(stats != nullptr)
    {
        ostream << "  Max nested level: " << stats->guess_max_nested_level << std::endl;
        if(stats->guess_max_nested_level > 0u)
        {
            ostream << "    > The solving of the grid required an hypothesis on " << stats->guess_total_calls << " row(s) or column(s)." << std::endl;
            ostream << "    > Max/total nb of alternatives being tested: " << stats->guess_max_alternatives << "/" << stats->guess_total_alternatives << std::endl;
        }
        ostream << "  Max theoretical nb of alternatives on a line: " << stats->max_theoretical_nb_alternatives << std::endl;
        ostream << "  " << stats->nb_reduce_all_rows_and_colums_calls << " calls to reduce_all_rows_and_columns()." << std::endl;
        ostream << "  " << stats->nb_reduce_lines_calls   << " calls to reduce_lines(). Max list size/total nb of lines being reduced: " << stats->max_reduce_list_size << "/" << stats->total_lines_reduced << std::endl;
        ostream << "  " << stats->nb_add_and_filter_calls << " calls to add_and_filter_lines(). Max list size/total nb of lines being added and filtered: " << stats->max_add_and_filter_list_size << "/" << stats->total_lines_added_and_filtered << std::endl;
        ostream << std::endl;
    }
}


std::vector<std::unique_ptr<SolvedGrid>> RefSolver::solve(const InputGrid& grid_input, GridStats* stats) const
{
    std::vector<std::unique_ptr<SolvedGrid>> solutions;

    if (stats != nullptr)
    {
        /* Reset stats */
        std::swap(*stats, GridStats());
    }

    Grid(grid_input, solutions, stats).solve();

    return solutions;
}


std::unique_ptr<Solver> get_ref_solver()
{
    return std::make_unique<RefSolver>();
}

} // namespace picross

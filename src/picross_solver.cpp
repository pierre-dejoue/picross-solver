/*******************************************************************************
 * PICROSS SOLVER: picross_solver.cpp
 *
 *   This file implements the core functions of the  Picross solver.
 *
 * Copyright (c) 2010 Pierre DEJOUE
 ******************************************************************************/
#include "picross_solver.h"


#include <algorithm>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <list>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>


namespace Tile
{
    char str(tile t)
    {
        if(t == UNKNOWN) { return '?'; }
        if(t == ZERO)    { return ' '; }
        if(t == ONE)     { return '#'; }
        std::ostringstream oss;
        oss << "Invalid tile value: " << t;
        throw std::invalid_argument(oss.str());
    }

    /* See comments for Line::add() */
    tile add(tile t1, tile t2)
    {
        if(t1 == t2 || t2 == Tile::UNKNOWN) { return t1; }
        else if(t1 == Tile::UNKNOWN)        { return t2; }
        else                                { throw PicrossLineAdditionError(); }
    }

    /* See comments for Line::reduce() */
    tile reduce(tile t1, tile t2)
    {
        if(t1 == t2) { return t1; }
        else         { return Tile::UNKNOWN; }
    }
} // end namespace Tile


Line::Line(Type type, std::vector<Tile::tile>& vect) :
    type(type),
    tiles(vect)
{
}


std::vector<Tile::tile>::const_iterator Line::read_tiles() const
{
    return tiles.begin();
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
void add_and_filter_lines(std::list<Line>& list_of_lines, const Line& filter_line, GridStats * stats)
{
    // Stats
    if(stats != 0)
    {
         int list_size = list_of_lines.size();
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
    if(stats != 0)
    {
         int list_size = list_of_lines.size();
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


bool Line::is_all_one_color(Tile::tile color) const
{
    for(size_t idx = 0; idx < tiles.size(); idx++)
    {
        if(tiles.at(idx) != color) { return false; }
    }
    return true;
}


void Line::print() const
{
    if(type == Line::ROW)    { std::cout << "ROW    "; }
    if(type == Line::COLUMN) { std::cout << "COLUMN "; }
    std::cout << str_line(*this) << std::endl;
}


std::string str_line(const Line& line)
{
    std::string out_str = "";
    for(int idx = 0; idx < line.get_size(); idx++) { out_str += Tile::str(line.get_tile(idx)); }
    return out_str;
}


std::string str_line_type(Line::Type type)
{
    if(type == Line::ROW)    { return "ROW"; }
    if(type == Line::COLUMN) { return "COLUMN"; }
    return "Unknonw :)";
}


Grid::Grid(int width, int height, const GridInput& grid_input, std::vector<Grid>& solutions, GridStats* in_stats, int n_level) :
    width(width),
    height(height),
    grid_input(grid_input),
    saved_solutions(solutions)
{
    // Allocate memory
    grid = new Tile::tile*[width];
    for(int x = 0; x < width; x++)
    {
        grid[x] = new Tile::tile[height];
        for(int y = 0; y < height; y++)
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

    // Stats
    stats = in_stats;
    nested_level = n_level;
    if(stats != 0 && nested_level > stats->guess_max_nested_level) {  stats->guess_max_nested_level = nested_level; }

    //cout << "Grid constructor" << endl;
}


/* Copy constructor. Do a full copy */
Grid::Grid(const Grid& c_grid) :
    width(c_grid.width),
    height(c_grid.height),
    grid_input(c_grid.grid_input),
    saved_solutions(c_grid.saved_solutions)
{
    // Allocate memory
    grid = new Tile::tile*[width];
    for(int x = 0; x < width; x++)
    {
        grid[x] = new Tile::tile[height];
    }

    // Copy the grid contents from this grid to the new one
    for(int x = 0; x < width; x++)
    {
        for(int y = 0; y < height; y++)
        {
            grid[x][y] = c_grid.grid[x][y];
        }
    }

    //cout << "Grid COPY constructor!" << endl;
}


/* Assignment operator. Do a full copy. */
Grid& Grid::operator=(const Grid& c_grid)
{
    // Allocate memory
    grid = new Tile::tile*[width];
    for(int x = 0; x < width; x++)
    {
        grid[x] = new Tile::tile[height];
    }

    // Copy the grid contents from this grid to the new one
    for(int x = 0; x < width; x++)
    {
        for(int y = 0; y < height; y++)
        {
            grid[x][y] = c_grid.grid[x][y];
        }
    }

    //cout << "Grid assignment!" << endl;

    return *this;
}


Grid::~Grid()
{
    // Free memory
    for (int x = 0; x < width; x++)
    {
       delete[] grid[x];
    }
    delete[] grid;

    //cout << "Grid destructor!" << endl;

}


Line Grid::get_line(Line::Type type, int index)
{
    if(type == Line::ROW)
    {
        std::vector<int> vect;
        for(int x = 0; x < width; x++) { vect.push_back(grid[x][index]); }
        return Line(type, vect);
    }
    else if(type == Line::COLUMN)
    {
        std::vector<int> vect(grid[index], grid[index] + height);
        return Line(type, vect);
    }
    else { throw std::invalid_argument("Grid::get_line: wrong type argument"); }
}


bool Grid::set(int x, int y, Tile::tile t)
{
    if(x < 0 || x >= width || y < 0 || y >= height) { throw std::invalid_argument("Grid::set: out of range"); }
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


bool Grid::set_line(Line line, int index)
{
    bool changed = false;
    if(line.get_type() == Line::ROW && line.get_size() == width)
    {
        for(int x = 0; x < width; x++) { changed |= set(x, index, line.get_tile(x)); }
    }
    else if(line.get_type() == Line::COLUMN && line.get_size() == height)
    {
        for(int y = 0; y < height; y++) { changed |= set(index, y, line.get_tile(y)); }
    }
    else
    {
        throw std::invalid_argument("Grid::set_line: wrong arguments");
    }
    return changed;
}


void Grid::print()
{
    std::cout << "Grid " << width << "x" << height << ":" << std::endl;
    for(int y = 0; y < height; y++)
    {
        Line line = get_line(Line::ROW, y);
        std::cout << "  " << str_line(line) << std::endl;
    }
    std::cout << std::endl;
}


bool Grid::reduce_one_line(Line::Type type, int index)
{
    Line filter_line = get_line(type, index);
    const Constraint * line_constraint;
    int line_size;

    if(type == Line::ROW)
    {
        line_constraint = &(grid_input.rows.at(index));
        line_size = width;
    }
    else if(type == Line::COLUMN)
    {
        line_constraint = &(grid_input.columns.at(index));
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
    alternatives[type].at(index) = all_lines.size();

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
    for(int x = 0; x < width; x++)
    {
        if(!line_complete[Line::COLUMN][x] && line_to_be_reduced[Line::COLUMN][x]) { changed |= reduce_one_line(Line::COLUMN, x); }
    }
    for(int y = 0; y < height; y++)
    {
        if(!line_complete[Line::ROW][y] && line_to_be_reduced[Line::ROW][y])       { changed |= reduce_one_line(Line::ROW, y); }
    }

    // Stats
    if(stats != 0) { stats->nb_reduce_all_rows_and_colums_calls++; }

    return changed;
}


bool Grid::is_solved()
{
    bool on_rows = std::accumulate(line_complete[Line::ROW].begin(), line_complete[Line::ROW].end(), true, std::logical_and<bool>());
    bool on_columns = std::accumulate(line_complete[Line::COLUMN].begin(), line_complete[Line::COLUMN].end(), true, std::logical_and<bool>());
    return on_rows || on_columns;
}


// Utility class used to find the min value, greater ot equal to 2. Used in Grid::solve()
class f_min_greater_than_2 {
private:
    int& min;
public:
    f_min_greater_than_2(int& min) : min(min) {}

    void operator()(int val)
    {
        if(val>=2)
        {
            if(min < 2 || val < min)
            {
                min = val;
            }
        }
    }
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
            std::vector<int>::iterator alternative_it;
            const Constraint* line_constraint;
            int line_size;
            int min_alt = -1;

            for_each(alternatives[Line::ROW].begin(),    alternatives[Line::ROW].end(),    f_min_greater_than_2(min_alt));
            for_each(alternatives[Line::COLUMN].begin(), alternatives[Line::COLUMN].end(), f_min_greater_than_2(min_alt));
            if(min_alt == -1) { throw std::logic_error("Grid::solve: min_alt = -1, this should not happen!"); }

            // Some stats
            if(stats != 0 && min_alt > stats->guess_max_alternatives)  { stats->guess_max_alternatives = min_alt; }
            if(stats != 0)                                             { stats->guess_total_alternatives += min_alt; }

            // Select one row or one column with the minimal number of alternatives.
            alternative_it = find_if(alternatives[Line::ROW].begin(), alternatives[Line::ROW].end(), [min_alt](const int alt) { return alt == min_alt; });
            if (alternative_it == alternatives[Line::ROW].end())
            {
                alternative_it = find_if(alternatives[Line::COLUMN].begin(), alternatives[Line::COLUMN].end(), [min_alt](const int alt) { return alt == min_alt; });
                if (alternative_it == alternatives[Line::COLUMN].end())
                {
                    // Again, this should not happen.
                    throw std::logic_error("Grid::solve: alternative_it = alternatives[Line::COLUMN].end(). Impossible!");
                }
                guess_line_type = Line::COLUMN;
                guess_line_index = alternative_it - alternatives[Line::COLUMN].begin();
                line_constraint = &(grid_input.columns.at(guess_line_index));
                line_size = height;
            }
            else
            {
                guess_line_type = Line::ROW;
                guess_line_index = alternative_it - alternatives[Line::ROW].begin();
                line_constraint = &(grid_input.rows.at(guess_line_index));
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
    if(stats != 0) { stats->guess_total_calls++; }
    for(std::list<Line>::const_iterator guess_line = guess_list_of_all_alternatives.begin(); guess_line != guess_list_of_all_alternatives.end(); guess_line++)
    {
        // Allocate a new grid.
        // One reason to not using the copy contructor: that would copy the guess_list_of_all_alternatives, which is not useful.
        Grid new_grid(width, height, grid_input, saved_solutions, stats, nested_level+1);

        // Copy the grid contents from this grid to the new one
        for(int x = 0; x < width; x++)
        {
            for(int y = 0; y < height; y++)
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
    saved_solutions.push_back(*this);
}


Constraint::Constraint(Line::Type type, const std::vector<int>& vect) :
    type(type),
    sets_of_ones(vect),
    sets_of_ones_plus_trailing_zero(vect)
{
    if (sets_of_ones_plus_trailing_zero.size() >= 2)
    {
        /* Increment by one all the elements of the array except the last one */
        std::transform(sets_of_ones_plus_trailing_zero.begin(), sets_of_ones_plus_trailing_zero.end() - 1, sets_of_ones_plus_trailing_zero.begin(), [](int i) { return i + 1; });
    }
    min_line_size = accumulate(sets_of_ones_plus_trailing_zero.begin(), sets_of_ones_plus_trailing_zero.end(), 0);
}


int Constraint::nb_filled_tiles() const
{
    return accumulate(sets_of_ones.begin(), sets_of_ones.end(), 0);
}


int Constraint::max_block_size() const
{
    // Sets_of_ones vector must not be empty.
    if(sets_of_ones.empty()) { return 0; }
    // Compute max
    return *max_element(sets_of_ones.begin(), sets_of_ones.end());
}


void Constraint::print() const
{
    std::cout << "Constraint on a " << str_line_type(type) << ": [ ";
    for (std::vector<int>::const_iterator i = sets_of_ones.begin(); i != sets_of_ones.end(); i++ )
    {
        std::cout << *i << " ";
    }
    std::cout << "]; min_line_size = " << min_line_size << std::endl;
}


static int nb_alternatives_for_fixed_nb_of_partitions(int nb_cells, int nb_partitions)
{
    if(nb_cells == 0 || nb_partitions <= 1)
    {
        return 1;
    }
    else
    {
        int accumulator = 0;
        for(int n = 0; n <= nb_cells; n++) { accumulator += nb_alternatives_for_fixed_nb_of_partitions(nb_cells - n, nb_partitions - 1); }
        return accumulator;
    }
}


int Constraint::theoretical_nb_alternatives(int size, GridStats * stats) const
{
    // Compute the number of alternatives for a line of size 'size', assuming all tiles are Tile::UNKONWN.
    // Number of zeros to add to the minimal size line.
    int nb_zeros = size - min_line_size;
    if(nb_zeros < 0)  { throw std::logic_error("Constraint::theoretical_nb_alternatives: line_size < min_line_size"); }

    int nb_alternatives = nb_alternatives_for_fixed_nb_of_partitions(nb_zeros, sets_of_ones.size() + 1);
    if(stats != 0 && nb_alternatives > stats->max_theoretical_nb_alternatives) {  stats->max_theoretical_nb_alternatives = nb_alternatives; }

    return nb_alternatives;
}


std::list<Line> Constraint::build_all_possible_lines_with_size(int size, const Line& filter_line, GridStats * stats) const
{
    if(filter_line.get_size() != size)   { throw std::invalid_argument("Wrong filter line size"); }
    if(filter_line.get_type() != type)   { throw std::invalid_argument("Wrong filter line type"); }

    // Number of zeros to add to the minimal size line.
    int nb_zeros = size - min_line_size;
    if(nb_zeros < 0)                     { throw std::logic_error("Constraint::build_all_possible_lines_with_size: line_size < min_line_size"); }

    std::list<Line> return_list;
    std::vector<Tile::tile> new_tile_vect(size, Tile::ZERO);

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
        for(int n = 0; n <= nb_zeros; n++)
        {
            for(int idx = 0; idx<n; idx++)                      { new_tile_vect.at(idx)   = Tile::ZERO; }
            for(int idx = 0; idx<sets_of_ones[0]; idx++)        { new_tile_vect.at(n+idx) = Tile::ONE;  }
            for(int idx = n + sets_of_ones[0]; idx<size; idx++) { new_tile_vect.at(idx)   = Tile::ZERO; }
            return_list.push_back( Line(type, new_tile_vect) );
        }

        // Filter the return_list against filter_line
        if(!filter_line.is_all_one_color(Tile::UNKNOWN)) { add_and_filter_lines(return_list, filter_line, stats); }
    }
    else
    {
        // For loop on the number of zeros before the first block of ones
        for(int n = 0; n <= nb_zeros; n++)
        {
            // Set the beginning of the line containing the first block of ones
            int begin_size = n + sets_of_ones_plus_trailing_zero[0];
            for(int idx = 0; idx<n; idx++)                      { new_tile_vect.at(idx)   = Tile::ZERO; }
            for(int idx = 0; idx<sets_of_ones[0]; idx++)        { new_tile_vect.at(n+idx) = Tile::ONE;  }
            new_tile_vect.at(begin_size-1) = Tile::ZERO;

            try
            {
                // Add the start of the line (first block of ones) and the beginning of the filter line
                std::vector<Tile::tile> begin_vect(new_tile_vect.begin(), new_tile_vect.begin()+ begin_size);
                std::vector<Tile::tile> begin_filter_vect(filter_line.read_tiles(), filter_line.read_tiles() + begin_size);

                Line begin_line(type, begin_vect);
                Line begin_filter(type, begin_filter_vect);
                begin_line.add(begin_filter);               // can throw PicrossLineAdditionError

                // If OK, then go on and recursively call this function to construct the remaining part of the line.
                std::vector<int> trim_sets_of_ones(sets_of_ones.begin()+1, sets_of_ones.end());
                Constraint  recursive_constraint(type, trim_sets_of_ones);

                std::vector<Tile::tile> end_filter_vect(filter_line.read_tiles()+ begin_size, filter_line.read_tiles() + filter_line.get_size());
                Line end_filter(type, end_filter_vect);

                std::list<Line> recursive_list =  recursive_constraint.build_all_possible_lines_with_size(size - begin_size, end_filter, stats);

                // Finally, construct the return_list based on the contents of the recursive_list.
                for(std::list<Line>::iterator line = recursive_list.begin(); line != recursive_list.end(); line++)
                {
                    copy(line->read_tiles(),  line->read_tiles() + line->get_size(), new_tile_vect.begin() + begin_size);
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


bool GridInput::sanity_check()
{
    // Sanity check of the grid input
    //  -> height != 0 and width != 0
    //  -> same number of painted cells on rows and columns
    //  -> for each constraint min_line_size is smaller than the width or height of the row or column, respectively.

    int width  = columns.size();
    int height = rows.size();

    if(height == 0 || width == 0)
    {
        std::cout << "Invalid width = " << width << " or height = " << height << std::endl;
        return false;
    }
    int nb_tiles_on_rows = 0, nb_tiles_on_columns = 0;
    for(std::vector<Constraint>::iterator c = rows.begin(); c != rows.end(); c++)
    {
        nb_tiles_on_rows += c->nb_filled_tiles();
        if(c->get_min_line_size() > width)
        {
            std::cout << "Width = " << width << " of the grid is too small for constraint: ";
            c->print();
            return false;
        }
    }
    for(std::vector<Constraint>::iterator c = columns.begin(); c != columns.end(); c++)
    {
        nb_tiles_on_columns += c->nb_filled_tiles();
        if(c->get_min_line_size() > height)
        {
            std::cout << "Height = " << height << " of the grid is too small for constraint: ";
            c->print();
            return false;
        }
    }
    if(nb_tiles_on_rows != nb_tiles_on_columns)
    {
        std::cout << "Number of filled tiles on rows (" << nb_tiles_on_rows << ") and columns (" <<  nb_tiles_on_columns << ") do not match." << std::endl;
        return false;
    }


    return true;
}


void reset_grid_stats(GridStats * stats)
{
    if(stats != 0)
    {
        memset(stats, 0, sizeof(GridStats));
    }
}


void print_grid_stats(GridStats * stats)
{
    if(stats != 0)
    {
        std::cout << "  Max nested level: " << stats->guess_max_nested_level << std::endl;
        if(stats->guess_max_nested_level != 0)
        {
            std::cout << "    > The solving of the grid required an hypothesis on " << stats->guess_total_calls << " row(s) or column(s)." << std::endl;
            std::cout << "    > Max/total nb of alternatives being tested: " << stats->guess_max_alternatives << "/" << stats->guess_total_alternatives << std::endl;
        }
        std::cout << "  Max theoretical nb of alternatives on a line: " << stats->max_theoretical_nb_alternatives << std::endl;
        std::cout << "  " << stats->nb_reduce_all_rows_and_colums_calls << " calls to reduce_all_rows_and_columns()." << std::endl;
        std::cout << "  " << stats->nb_reduce_lines_calls   << " calls to reduce_lines(). Max list size/total nb of lines being reduced: " << stats->max_reduce_list_size << "/" << stats->total_lines_reduced << std::endl;
        std::cout << "  " << stats->nb_add_and_filter_calls << " calls to add_and_filter_lines(). Max list size/total nb of lines being added and filtered: " << stats->max_add_and_filter_list_size << "/" << stats->total_lines_added_and_filtered << std::endl;
        std::cout << std::endl;
    }
}

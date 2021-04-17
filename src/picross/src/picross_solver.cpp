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
#include <iomanip>
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

    class PicrossLineDeltaError : public std::runtime_error
    {
    public:
        PicrossLineDeltaError() : std::runtime_error("[PicrossSolver] An error was detected computing the delta of two lines")
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
        if (t == UNKNOWN) { return '?'; }
        if (t == ZERO)    { return ' '; }
        if (t == ONE)     { return '#'; }
        std::ostringstream oss;
        oss << "Invalid tile value: " << t;
        throw std::invalid_argument(oss.str());
    }

    inline Type compatible(Type t1, Type t2)
    {
        return t1 == Tile::UNKNOWN || t2 == Tile::UNKNOWN || t1 == t2;
    }

    inline Type add(Type t1, Type t2)
    {
        if (t1 == t2 || t2 == Tile::UNKNOWN) { return t1; }
        else if (t1 == Tile::UNKNOWN)        { return t2; }
        else                                 { throw PicrossLineAdditionError(); }
    }

    inline Type delta(Type t1, Type t2)
    {
        if (t1 == t2) { return Tile::UNKNOWN; }
        else if (t1 == Tile::UNKNOWN) { return t2; }
        else { throw PicrossLineDeltaError(); }
    }

    inline Type reduce(Type t1, Type t2)
    {
        if (t1 == t2) { return t1; }
        else          { return Tile::UNKNOWN; }
    }
} // end namespace Tile


Line::Line(Line::Type type, size_t index, const std::vector<Tile::Type>& tiles) :
    type(type),
    index(index),
    tiles(tiles)
{
}


Line::Line(Line::Type type, size_t index, std::vector<Tile::Type>&& tiles) :
    type(type),
    index(index),
    tiles(std::move(tiles))
{
}


Line::Type Line::get_type() const
{
    return type;
}


size_t Line::get_index() const
{
    return index;
}


const std::vector<Tile::Type>& Line::get_tiles() const
{
    return tiles;
}


size_t Line::size() const
{
    return tiles.size();
}


Tile::Type Line::at(size_t idx) const
{
    return tiles.at(idx);
}


/* Line::add combines the information of two lines into a single one.
 * Return false if the lines do not match (e.g. a tile is empty in one line and filled in in the other).
 *
 * Example:
 *         line1:    ....##??????
 *         line2:    ..????##..??
 * line1 + line2:    ....####..??
 */
bool Line::add(const Line& line)
{
    if (line.type != type)                 { throw std::invalid_argument("add: Line type mismatch"); }
    if (line.index != index)               { throw std::invalid_argument("add: Line index mismatch"); }
    if (line.tiles.size() != tiles.size()) { throw std::invalid_argument("add: Line size mismatch"); }
    bool valid = true;
    for (size_t idx = 0u; idx < tiles.size(); ++idx)
    {
        if (!Tile::compatible(line.tiles.at(idx), tiles[idx]))
        {
            valid = false;
            break;
        }
    }
    if (valid)
    {
        std::transform(tiles.cbegin(), tiles.cend(), line.tiles.cbegin(), tiles.begin(), Tile::add);
    }
    return valid;
}


/* Line::reduce captures the information that is common to two lines.
 *
 * Example:
 *         line1:    ??..######..
 *         line2:    ??....######
 * line1 ^ line2:    ??..??####??
 */
void Line::reduce(const Line& line)
{
    if (line.type != type)                 { throw std::invalid_argument("reduce: Line type mismatch"); }
    if (line.index != index)               { throw std::invalid_argument("reduce: Line index mismatch"); }
    if (line.tiles.size() != tiles.size()) { throw std::invalid_argument("reduce: Line size mismatch"); }
    std::transform(tiles.begin(), tiles.end(), line.tiles.begin(), tiles.begin(), Tile::reduce);
}


/* line_delta computes the delta between line2 and line1, such that line2 = line1 + delta
 *
 * Example:
 *  (this) line2:    ....####..??
 *         line1:    ..????##..??
 *         delta:    ??..##??????
 */
Line line_delta(const Line& line1, const Line& line2)
{
    if (line1.get_type() != line2.get_type()) { throw std::invalid_argument("line_delta: Line type mismatch"); }
    if (line1.get_index() != line2.get_index()) { throw std::invalid_argument("line_delta: Line index mismatch"); }
    if (line1.size() != line2.size()) { throw std::invalid_argument("line_delta: Line size mismatch"); }
    std::vector<Tile::Type> delta_tiles;
    delta_tiles.resize(line1.size(), Tile::UNKNOWN);
    std::transform(line1.get_tiles().cbegin(), line1.get_tiles().cend(), line2.get_tiles().cbegin(), delta_tiles.begin(), Tile::delta);
    return Line(line1.get_type(), line1.get_index(), std::move(delta_tiles));
}


/* Add (with Tile::add) a filter line to each line in a list. If the addition fails,
 * the offending line is discarded from the list
 */
void add_and_filter_lines(std::list<Line>& lines, const Line& filter_line, GridStats* stats)
{
    // Stats
    if (stats != nullptr)
    {
         const auto list_size = static_cast<unsigned int>(lines.size());
         stats->nb_add_and_filter_calls++;
         stats->total_lines_added_and_filtered += list_size;
         if (list_size > stats->max_add_and_filter_list_size) { stats->max_add_and_filter_list_size = list_size; }
    }

    std::list<Line>::iterator it = lines.begin();
    while(it != lines.end())
    {
        const bool match = it->add(filter_line);
        it = match ? std::next(it) : lines.erase(it);
    }
}


/* Reduce a list of lines and return one line that comprises the information common to all */
Line reduce_line(const std::list<Line>& all_alternatives, GridStats * stats)
{
    // Stats
    if (stats != nullptr)
    {
         const auto list_size = static_cast<unsigned int>(all_alternatives.size());
         stats->nb_reduce_line_calls++;
         stats->total_lines_reduced += list_size;
         if (list_size > stats->max_reduce_list_size) { stats->max_reduce_list_size = list_size; }
    }

    if (all_alternatives.empty()) { throw std::invalid_argument("Cannot reduce an empty list of lines"); }
    Line new_line = all_alternatives.front();
    for (std::list<Line>::const_iterator line = ++all_alternatives.begin(); line != all_alternatives.end(); line++)
    {
        new_line.reduce(*line);
    }
    return new_line;
}


bool is_all_one_color(const Line& line, Tile::Type color)
{
    return std::all_of(line.get_tiles().cbegin(), line.get_tiles().cend(), [color](const Tile::Type t) { return t == color; });
}


std::string str_line(const Line& line)
{
    std::string out_str;
    for (unsigned int idx = 0u; idx < line.size(); idx++) { out_str += Tile::str(line.at(idx)); }
    return out_str;
}


std::string str_line_type(Line::Type type)
{
    if (type == Line::ROW) { return "ROW"; }
    if (type == Line::COL) { return "COL"; }
    assert(0);
    return "UNKNOWN";
}


std::ostream& operator<<(std::ostream& ostream, const Line& line)
{
    std::ios prev_iostate(nullptr);
    prev_iostate.copyfmt(ostream);
    ostream << str_line_type(line.get_type()) << " " << std::setw(3) << line.get_index() << " " << str_line(line);
    ostream.copyfmt(prev_iostate);
    return ostream;
}


namespace
{
    std::vector<Constraint> row_constraints_from(const InputGrid& grid)
    {
        std::vector<Constraint> rows;
        rows.reserve(grid.rows.size());
        std::transform(grid.rows.cbegin(), grid.rows.cend(), std::back_inserter(rows), [](const InputGrid::Constraint& c) { return Constraint(Line::ROW, c); });
        return rows;
    }

    std::vector<Constraint> column_constraints_from(const InputGrid& grid)
    {
        std::vector<Constraint> cols;
        cols.reserve(grid.cols.size());
        std::transform(grid.cols.cbegin(), grid.cols.cend(), std::back_inserter(cols), [](const InputGrid::Constraint& c) { return Constraint(Line::COL, c); });
        return cols;
    }
}


const std::string& OutputGrid::get_name() const
{
    return name;
}


OutputGrid::OutputGrid(size_t width, size_t height, const std::string& name) :
    width(width),
    height(height),
    name(name),
    grid(height * width, Tile::UNKNOWN)
{
}


OutputGrid& OutputGrid::operator=(const OutputGrid& other)
{
    assert(width == other.width);
    assert(height == other.height);
    assert(name == other.name);
    grid = other.grid;
    return *this;
}


OutputGrid& OutputGrid::operator=(OutputGrid&& other)
{
    assert(width == other.width);
    assert(height == other.height);
    assert(name == other.name);
    grid = std::move(other.grid);
    return *this;
}


size_t OutputGrid::get_width() const
{
    return width;
}


size_t OutputGrid::get_height() const
{
    return height;
}


Tile::Type OutputGrid::get(size_t x, size_t y) const
{
    if (x >= width) { throw std::invalid_argument("OutputGrid::set: x is out of range"); }
    if (y >= height) { throw std::invalid_argument("OutputGrid::set: y is out of range"); }
    return grid[x * height + y];
}


void OutputGrid::set(size_t x, size_t y, Tile::Type t)
{
    if (x >= width) { throw std::invalid_argument("OutputGrid::set: x is out of range"); }
    if (y >= height) { throw std::invalid_argument("OutputGrid::set: y is out of range"); }
    grid[x * height + y] = t;
}


Line OutputGrid::get_line(Line::Type type, size_t index) const
{
    std::vector<Tile::Type> tiles;
    if (type == Line::ROW)
    {
        if (index >= height) { throw std::invalid_argument("Line ctor: row index is out of range"); }
        tiles.reserve(width);
        for (unsigned int x = 0u; x < width; x++)
        {
            tiles.push_back(get(x, index));
        }
    }
    else if (type == Line::COL)
    {
        if (index >= width) { throw std::invalid_argument("Line ctor: column index is out of range"); }
        tiles.reserve(height);
        tiles.insert(tiles.cbegin(), grid.data() + index * height, grid.data() + (index + 1) * height);
    }
    else
    {
        throw std::invalid_argument("Line ctor: wrong type argument");
    }
    return Line(type, index, std::move(tiles));
}


bool OutputGrid::is_solved() const
{
    return std::none_of(grid.cbegin(), grid.cend(), [](const Tile::Type& t) { return t == Tile::UNKNOWN; });
}


std::ostream& operator<<(std::ostream& ostream, const OutputGrid& grid)
{
    ostream << "Grid " << grid.get_width() << "x" << grid.get_height() << ":" << std::endl;
    for (unsigned int y = 0u; y < grid.get_height(); y++)
    {
        const Line row = grid.get_line(Line::ROW, y);
        ostream << "  " << str_line(row) << std::endl;
    }
    return ostream;
}


WorkGrid::WorkGrid(const InputGrid& grid, Solver::Solutions* solutions, GridStats* stats, Solver::Observer observer) :
    OutputGrid(grid.cols.size(), grid.rows.size(), grid.name),
    rows(row_constraints_from(grid)),
    cols(column_constraints_from(grid)),
    saved_solutions(solutions),
    stats(stats),
    observer(std::move(observer)),
    nested_level(0u)
{
    assert(cols.size() == get_width());
    assert(rows.size() == get_height());
    assert(saved_solutions != nullptr);

    // Other initializations
    line_completed[Line::ROW].resize(get_height(), false);
    line_completed[Line::COL].resize(get_width(), false);
    line_to_be_reduced[Line::ROW].resize(get_height(), true);
    line_to_be_reduced[Line::COL].resize(get_width(), true);
    nb_alternatives[Line::ROW].resize(get_height(), 0u);
    nb_alternatives[Line::COL].resize(get_width(), 0u);
}


// Shallow copy (does not copy the list of alternatives)
WorkGrid::WorkGrid(const WorkGrid& parent, unsigned int nested_level) :
    OutputGrid(static_cast<const OutputGrid&>(parent)),
    rows(parent.rows),
    cols(parent.cols),
    saved_solutions(parent.saved_solutions),
    stats(parent.stats),
    observer(parent.observer),
    nested_level(nested_level)
{
    assert(nested_level > 0u);

    line_completed[Line::ROW] = parent.line_completed[Line::ROW];
    line_completed[Line::COL] = parent.line_completed[Line::COL];

    line_to_be_reduced[Line::ROW] = parent.line_to_be_reduced[Line::ROW];
    line_to_be_reduced[Line::COL] = parent.line_to_be_reduced[Line::COL];

    nb_alternatives[Line::ROW].resize(get_height(), 0u);
    nb_alternatives[Line::COL].resize(get_width(), 0u);

    // Stats
    if (stats != nullptr && nested_level > stats->max_nested_level)
    {
        stats->max_nested_level = nested_level;
    }

    // Solver::Observer
    if (observer)
    {
        observer(Solver::Event::BRANCHING, nullptr, nested_level);
    }
}


bool WorkGrid::set_w_reduce_flag(size_t x, size_t y, Tile::Type t)
{
    if (t != get(x, y))
    {
        assert(get(x, y) == Tile::UNKNOWN);

        // modify grid
        set(x, y, t);

        // mark the impacted row and column with flag "to be reduced".
        line_to_be_reduced[Line::ROW][y] = true;
        line_to_be_reduced[Line::COL][x] = true;

        // return true since the grid was modifed.
        return true;
    }
    else
    {
        return false;
    }
}


bool WorkGrid::set_line(const Line& line)
{
    bool changed = false;
    const size_t index = line.get_index();
    if (observer)
    {
        const Line origin_line = get_line(line.get_type(), index);
        const Line delta = line_delta(origin_line, line);
        observer(Solver::Event::DELTA_LINE, &delta, nested_level);
    }
    const auto width = static_cast<unsigned int>(get_width());
    const auto height = static_cast<unsigned int>(get_height());
    if (line.get_type() == Line::ROW && line.size() == width)
    {
        for (unsigned int x = 0u; x < width; x++) { changed |= set_w_reduce_flag(x, index, line.at(x)); }
    }
    else if (line.get_type() == Line::COL && line.size() == height)
    {
        for (unsigned int y = 0u; y < height; y++) { changed |= set_w_reduce_flag(index, y, line.at(y)); }
    }
    else
    {
        throw std::invalid_argument("WorkGrid::set_line: wrong arguments");
    }
    return changed;
}


bool WorkGrid::single_line_pass(Line::Type type, unsigned int index)
{
    assert(line_completed[type].at(index) == false);
    const Constraint * line_constraint = type == Line::ROW ? &(rows.at(index)) : &(cols.at(index));
    assert(line_constraint != nullptr);

    // Stats
    if (stats != nullptr) { stats->nb_single_line_pass_calls++;  }

    // If the color of every tile in the line is unknown, there is a simple way to determine
    // if the reduce operation is relevant: max block size must be greater than line_size - min_line_size.
    const Line filter_line = get_line(type, index);
    if (is_all_one_color(filter_line, Tile::UNKNOWN))
    {
        if (line_constraint->max_block_size() != 0 &&
            line_constraint->max_block_size() <= (filter_line.size() - line_constraint->get_min_line_size()))
        {
            // Need not reduce this line until one of the tiles is modified.
            line_to_be_reduced[type][index] = false;

            // We shall compute the number of alternatives anyway (in case we have to make an hypothesis on the grid)
            nb_alternatives[type].at(index) = line_constraint->theoretical_nb_alternatives(filter_line.size(), stats);

            // No change made on this line
            return false;
        }
    }

    // Compute all possible lines that match the data already present in the grid and the line constraints
    std::list<Line> all_lines = line_constraint->build_all_possible_lines(filter_line, stats);
    nb_alternatives[type].at(index) = static_cast<unsigned int>(all_lines.size());

    // If the list of all lines is empty, it means the grid data is contradictory. Throw an exception.
    if (nb_alternatives[type].at(index) == 0) { throw PicrossGridCannotBeSolved(); }

    // If the list comprises of only one element, it means we solved that line
    if (nb_alternatives[type].at(index) == 1) { line_completed[type].at(index) = true; }

    // In any case, update the grid data with the reduced line resulting from list all_lines
    const Line new_line = reduce_line(all_lines, stats);
    bool changed = set_line(new_line);

    // This line does not need to be reduced until one of the tiles is modified.
    line_to_be_reduced[type][index] = false;

    // return true if some tile was modified. False otherwise.
    return changed;
}


bool WorkGrid::full_grid_pass()
{
    // Stats
    if (stats != nullptr) { stats->nb_full_grid_pass_calls++; }

    // Reduce all columns and all rows. Return false if no change was made on the grid.
    bool changed = false;
    for (unsigned int x = 0u; x < get_width(); x++)
    {
        if (line_to_be_reduced[Line::COL][x]) { changed |= single_line_pass(Line::COL, x); }
    }
    for (unsigned int y = 0u; y < get_height(); y++)
    {
        if (line_to_be_reduced[Line::ROW][y]) { changed |= single_line_pass(Line::ROW, y); }
    }
    return changed;
}


bool WorkGrid::all_lines_completed() const
{
    const bool all_rows = std::all_of(line_completed[Line::ROW].cbegin(), line_completed[Line::ROW].cend(), [](bool b) { return b; });
    const bool all_cols = std::all_of(line_completed[Line::COL].cbegin(), line_completed[Line::COL].cend(), [](bool b) { return b; }); ;
    return all_rows || all_cols;
}


bool WorkGrid::solve()
{
    try
    {
        while(full_grid_pass())
        {
            // While the reduce method is successful, use it to find the filled and empty tiles.
        }

        // Are we done?
        if (all_lines_completed())
        {
            if (observer)
            {
                observer(Solver::Event::SOLVED_GRID, nullptr, nested_level);
            }
            save_solution();
            return true;
        }
        // If we are not, we have to make an hypothesis and continue based on that
        else
        {
            // Find the row or column not yet solved with the minimal alternative lines.
            // That is the min of all alternatives greater or equal to 2.
            unsigned int min_alt = 0u;
            for (const auto& type : { Line::ROW, Line::COL })
            {
                for (const unsigned int val : nb_alternatives[type])
                {
                    if (val >= 2u && (min_alt < 2u || val < min_alt))
                    {
                        min_alt = val;
                    }
                }
            }

            if (min_alt == 0u) { throw std::logic_error("WorkGrid::solve: no min value, this should not happen!"); }

            // Some stats
            if (stats != nullptr && min_alt > stats->guess_max_alternatives)  { stats->guess_max_alternatives = min_alt; }
            if (stats != nullptr)                                             { stats->guess_total_alternatives += min_alt; }

            // Select one row or one column with the minimal number of alternatives.
            const Constraint* line_constraint = nullptr;
            auto alternative_it = std::find_if (nb_alternatives[Line::ROW].cbegin(), nb_alternatives[Line::ROW].cend(), [min_alt](unsigned int alt) { return alt == min_alt; });
            if (alternative_it == nb_alternatives[Line::ROW].end())
            {
                alternative_it = std::find_if (nb_alternatives[Line::COL].cbegin(), nb_alternatives[Line::COL].cend(), [min_alt](unsigned int alt) { return alt == min_alt; });
                if (alternative_it == nb_alternatives[Line::COL].end())
                {
                    // Again, this should not happen.
                    throw std::logic_error("WorkGrid::solve: alternative_it = alternatives[Line::COL].end(). Impossible!");
                }
                guess_line_type = Line::COL;
                guess_line_index = static_cast<unsigned int>(alternative_it - nb_alternatives[Line::COL].cbegin());
                line_constraint = &(cols.at(guess_line_index));
            }
            else
            {
                guess_line_type = Line::ROW;
                guess_line_index = static_cast<unsigned int>(alternative_it - nb_alternatives[Line::ROW].cbegin());
                line_constraint = &(rows.at(guess_line_index));
            }

            assert(line_constraint != nullptr);
            guess_list_of_all_alternatives = line_constraint->build_all_possible_lines(get_line(guess_line_type, guess_line_index), stats);

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


bool WorkGrid::guess() const
{
    /* This function will test a range of alternatives for one particular line of the grid, each time
     * creating a new instance of the grid class on which the function WorkGrid::solve() is called.
     */
    bool flag_solution_found = false;
    if (stats != nullptr) { stats->guess_total_calls++; }
    for (const Line& guess_line : guess_list_of_all_alternatives)
    {
        // Allocate a new work grid. Use the shallow copy.
        WorkGrid new_grid(*this, nested_level + 1);

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        if (!new_grid.set_line(guess_line)) { throw std::logic_error("WorkGrid::guess: no change in the new grid, will cause infinite loop."); }
        new_grid.line_completed[guess_line.get_type()].at(guess_line_index) = true;
        new_grid.line_to_be_reduced[guess_line.get_type()].at(guess_line_index) = false;

        // Solve the new grid!
        flag_solution_found |= new_grid.solve();
    }
    return flag_solution_found;
}


void WorkGrid::save_solution() const
{
    assert(is_solved());

    // Shallow copy of only the grid data
    saved_solutions->emplace_back(static_cast<const OutputGrid&>(*this));
}


Constraint::Constraint(Line::Type type, const InputGrid::Constraint& vect) :
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
        min_line_size = std::accumulate(sets_of_ones.cbegin(), sets_of_ones.cend(), 0u) + static_cast<unsigned int>(sets_of_ones.size()) - 1u;
    }
}


unsigned int Constraint::nb_filled_tiles() const
{
    return std::accumulate(sets_of_ones.cbegin(), sets_of_ones.cend(), 0u);
}


unsigned int Constraint::max_block_size() const
{
    // Sets_of_ones vector must not be empty.
    if (sets_of_ones.empty()) { return 0u; }

    // Compute max
    return *max_element(sets_of_ones.cbegin(), sets_of_ones.cend());
}


void Constraint::print(std::ostream& ostream) const
{
    ostream << "Constraint on a " << str_line_type(type) << ": [ ";
    for (auto& it = sets_of_ones.begin(); it != sets_of_ones.end(); ++it)
    {
        ostream << *it << " ";
    }
    ostream << "]; min_line_size = " << min_line_size;
}


std::ostream& operator<<(std::ostream& ostream, const Constraint& constraint)
{
    constraint.print(ostream);
    return ostream;
}


int Constraint::theoretical_nb_alternatives(unsigned int line_size, GridStats * stats) const
{
    // Compute the number of alternatives for a line of size 'size', assuming all tiles are Tile::UNKONWN.
    // Number of zeros to add to the minimal size line.
    if (line_size < min_line_size)  { throw std::logic_error("Constraint::theoretical_nb_alternatives: line_size < min_line_size"); }
    unsigned int nb_zeros = line_size - min_line_size;

    unsigned int nb_alternatives = nb_alternatives_for_fixed_nb_of_partitions(nb_zeros, static_cast<unsigned int>(sets_of_ones.size()) + 1);
    if (stats != nullptr && nb_alternatives > stats->max_theoretical_nb_alternatives) {  stats->max_theoretical_nb_alternatives = nb_alternatives; }

    return nb_alternatives;
}


std::list<Line> Constraint::build_all_possible_lines(const Line& filter_line, GridStats* stats) const
{
    if (filter_line.get_type() != type) { throw std::invalid_argument("Wrong filter line type"); }
    const size_t index = filter_line.get_index();

    // Number of zeros to add to the minimal size line.
    if (filter_line.size() < min_line_size) { throw std::logic_error("Constraint::build_all_possible_lines_with_size: line_size < min_line_size"); }
    unsigned int nb_zeros = filter_line.size() - min_line_size;

    std::list<Line> return_list;
    std::vector<Tile::Type> new_tile_vect(filter_line.size(), Tile::ZERO);

    if (sets_of_ones.size() == 0)
    {
        // Return a list with only one all-zero line
        return_list.emplace_back(type, index, new_tile_vect);

        // Filter the return_list against input line
        add_and_filter_lines(return_list, filter_line, stats);
    }
    else if (sets_of_ones.size() == 1)
    {
        // Build the list of all possible lines with only one block of continuous filled tiles.
        // NB: in this case nb_zeros = size - sets_of_ones[0]
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            for (unsigned int idx = 0u; idx < n; idx++)                                      { new_tile_vect.at(idx)   = Tile::ZERO; }
            for (unsigned int idx = 0u; idx < sets_of_ones[0]; idx++)                        { new_tile_vect.at(n+idx) = Tile::ONE;  }
            for (unsigned int idx = n + sets_of_ones[0]; idx < filter_line.size(); idx++)    { new_tile_vect.at(idx) = Tile::ZERO; }
            return_list.emplace_back(type, index, new_tile_vect);
        }

        // Filter the return_list against filter_line
        if (!is_all_one_color(filter_line, Tile::UNKNOWN)) { add_and_filter_lines(return_list, filter_line, stats); }
    }
    else
    {
        // For loop on the number of zeros before the first block of ones
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            for (unsigned int idx = 0u; idx < n; idx++)                  { new_tile_vect.at(idx)   = Tile::ZERO; }
            for (unsigned int idx = 0u; idx < sets_of_ones[0]; idx++)    { new_tile_vect.at(n+idx) = Tile::ONE;  }
            const unsigned int begin_size = n + sets_of_ones[0] + 1u;
            new_tile_vect.at(begin_size - 1u) = Tile::ZERO;

            // Add the start of the line (first block of ones) and the beginning of the filter line
            std::vector<Tile::Type> begin_vect(new_tile_vect.begin(), new_tile_vect.begin() + begin_size);
            std::vector<Tile::Type> begin_filter_vect(filter_line.get_tiles().cbegin(), filter_line.get_tiles().cbegin() + begin_size);

            Line begin_line(type, index, begin_vect);
            Line begin_filter(type, index, begin_filter_vect);
            if (begin_line.add(begin_filter))
            {
                // If OK, then go on and recursively call this function to construct the remaining part of the line.
                std::vector<unsigned int> trim_sets_of_ones(sets_of_ones.begin() + 1, sets_of_ones.end());
                Constraint recursive_constraint(type, trim_sets_of_ones);

                std::vector<Tile::Type> end_filter_vect(filter_line.get_tiles().cbegin() + begin_size, filter_line.get_tiles().cend());
                Line end_filter(type, index, end_filter_vect);

                std::list<Line> recursive_list = recursive_constraint.build_all_possible_lines(end_filter, stats);

                // Finally, construct the return_list based on the contents of the recursive_list.
                for (const Line& line : recursive_list)
                {
                    std::copy(line.get_tiles().cbegin(), line.get_tiles().cend(), new_tile_vect.begin() + begin_size);
                    return_list.emplace_back(type, index, new_tile_vect);
                }
            }
            else
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

    const auto width  = static_cast<unsigned int>(grid_input.cols.size());
    const auto height = static_cast<unsigned int>(grid_input.rows.size());

    if (height == 0u)
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
    for (auto& c = grid_input.rows.begin(); c != grid_input.rows.end(); c++)
    {
        Constraint row(Line::ROW, *c);
        nb_tiles_on_rows += row.nb_filled_tiles();
        if (row.get_min_line_size() > width)
        {
            std::ostringstream oss;
            oss << "Width = " << width << " of the grid is too small for constraint: " << row << std::endl;
            return std::make_pair(false, oss.str());
        }
    }
    unsigned int nb_tiles_on_cols = 0u;
    for (auto& c = grid_input.cols.begin(); c != grid_input.cols.end(); c++)
    {
        Constraint col(Line::COL, *c);
        nb_tiles_on_cols += col.nb_filled_tiles();
        if (col.get_min_line_size() > height)
        {
            std::ostringstream oss;
            oss << "Height = " << height << " of the grid is too small for constraint: " << col << std::endl;
            return std::make_pair(false, oss.str());
        }
    }
    if (nb_tiles_on_rows != nb_tiles_on_cols)
    {
        std::ostringstream oss;
        oss << "Number of filled tiles on rows (" << nb_tiles_on_rows << ") and columns (" <<  nb_tiles_on_cols << ") do not match." << std::endl;
        return std::make_pair(false, oss.str());
    }
    return std::make_pair(true, std::string());
}


std::ostream& operator<<(std::ostream& ostream, const GridStats& stats)
{
    ostream << "  Max nested level: " << stats.max_nested_level << std::endl;
    if (stats.max_nested_level > 0u)
    {
        ostream << "    > The solving of the grid required an hypothesis on " << stats.guess_total_calls << " row(s) or column(s)." << std::endl;
        ostream << "    > Max/total nb of alternatives being tested: " << stats.guess_max_alternatives << "/" << stats.guess_total_alternatives << std::endl;
    }
    ostream << "  Max theoretical nb of alternatives on a line: " << stats.max_theoretical_nb_alternatives << std::endl;
    ostream << "  " << stats.nb_full_grid_pass_calls << " calls to full_grid_pass()." << std::endl;
    ostream << "  " << stats.nb_single_line_pass_calls << " calls to single_line_pass()." << std::endl;
    ostream << "  " << stats.nb_reduce_line_calls << " calls to reduce_line(). Max list size/total nb of lines reduced: " << stats.max_reduce_list_size << "/" << stats.total_lines_reduced << std::endl;
    ostream << "  " << stats.nb_add_and_filter_calls << " calls to add_and_filter_lines(). Max list size/total nb of lines being added and filtered: " << stats.max_add_and_filter_list_size << "/" << stats.total_lines_added_and_filtered << std::endl;

    return ostream;
}


Solver::Solutions RefSolver::solve(const InputGrid& grid_input, GridStats* stats) const
{
    Solutions solutions;

    if (stats != nullptr)
    {
        /* Reset stats */
        std::swap(*stats, GridStats());
    }

    const bool success = WorkGrid(grid_input, &solutions, stats, observer).solve();

    assert(success == (solutions.size() > 0u));
    return solutions;
}


void RefSolver::set_observer(Observer observer)
{
    this->observer = std::move(observer);
}


std::ostream& operator<<(std::ostream& ostream, Solver::Event event)
{
    switch (event)
    {
    case Solver::Event::BRANCHING:
        ostream << "BRANCHING";
        break;
    case Solver::Event::DELTA_LINE:
        ostream << "DELTA_LINE";
        break;
    case Solver::Event::SOLVED_GRID:
        ostream << "SOLVED_GRID";
        break;
    default:
        throw std::invalid_argument("Unknown Solver::Event");
    }
    return ostream;
}


std::unique_ptr<Solver> get_ref_solver()
{
    return std::make_unique<RefSolver>();
}

} // namespace picross

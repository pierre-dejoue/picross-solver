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
#include <limits>
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


Line::Line(Line::Type type, size_t index, size_t size, Tile::Type init_tile) :
    type(type),
    index(index),
    tiles(size, init_tile)
{
}


Line::Line(const Line& other, Tile::Type init_tile) :
    Line(other.type, other.index, other.size(), init_tile)
{
}


Line::Line(Line::Type type, size_t index, const Line::Container& tiles) :
    type(type),
    index(index),
    tiles(tiles)
{
}


Line::Line(Line::Type type, size_t index, Line::Container&& tiles) :
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


const Line::Container& Line::get_tiles() const
{
    return tiles;
}


Line::Container& Line::get_tiles()
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

/* Line::compatible() tests if two lines are compatible with each other
 */
bool Line::compatible(const Line& other) const
{
    if (other.type != type) { throw std::invalid_argument("compatible: Line type mismatch"); }
    if (other.index != index) { throw std::invalid_argument("compatible: Line index mismatch"); }
    if (other.tiles.size() != tiles.size()) { throw std::invalid_argument("compatible: Line size mismatch"); }
    for (size_t idx = 0u; idx < tiles.size(); ++idx)
    {
        if (!Tile::compatible(other.tiles.at(idx), tiles[idx]))
        {
            return false;
        }
    }
    return true;
}

/* Line::add() combines the information of two lines into a single one.
 * Return false if the lines are not compatible, in which case 'this' is not modified.
 *
 * Example:
 *         line1:    ....##??????
 *         line2:    ..????##..??
 * line1 + line2:    ....####..??
 */
bool Line::add(const Line& other)
{
    if (other.type != type)                 { throw std::invalid_argument("add: Line type mismatch"); }
    if (other.index != index)               { throw std::invalid_argument("add: Line index mismatch"); }
    if (other.tiles.size() != tiles.size()) { throw std::invalid_argument("add: Line size mismatch"); }
    const bool valid = compatible(other);
    if (valid)
    {
        std::transform(tiles.cbegin(), tiles.cend(), other.tiles.cbegin(), tiles.begin(), Tile::add);
    }
    return valid;
}


/* Line::reduce() captures the information that is common to two lines.
 *
 * Example:
 *         line1:    ??..######..
 *         line2:    ??....######
 * line1 ^ line2:    ??..??####??
 */
void Line::reduce(const Line& other)
{
    if (other.type != type)                 { throw std::invalid_argument("reduce: Line type mismatch"); }
    if (other.index != index)               { throw std::invalid_argument("reduce: Line index mismatch"); }
    if (other.tiles.size() != tiles.size()) { throw std::invalid_argument("reduce: Line size mismatch"); }
    std::transform(tiles.begin(), tiles.end(), other.tiles.begin(), tiles.begin(), Tile::reduce);
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


bool is_all_one_color(const Line& line, Tile::Type color)
{
    return std::all_of(line.get_tiles().cbegin(), line.get_tiles().cend(), [color](const Tile::Type t) { return t == color; });
}

bool is_fully_defined(const Line& line)
{
    return std::none_of(line.get_tiles().cbegin(), line.get_tiles().cend(), [](const Tile::Type t) { return t == Tile::UNKNOWN; });
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


bool operator==(const Line& lhs, const Line& rhs)
{
    return lhs.get_type() == rhs.get_type()
        && lhs.get_index() == rhs.get_index()
        && lhs.get_tiles() == rhs.get_tiles();
}


bool operator!=(const Line& lhs, const Line& rhs)
{
    return !(lhs == rhs);
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
    grid()
{
    reset();
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


void OutputGrid::reset()
{
    std::swap(grid, std::vector<Tile::Type>(height * width, Tile::UNKNOWN));
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


BinomialCoefficientsCache::Rep BinomialCoefficientsCache::overflowValue()
{
    return std::numeric_limits<Rep>::max();
}


unsigned int BinomialCoefficientsCache::nb_alternatives_for_fixed_nb_of_partitions(unsigned int nb_elts, unsigned int nb_buckets)
{
    assert(nb_buckets > 0u);
    const unsigned int k = nb_buckets - 1;
    const unsigned int n = nb_elts + k;
    if (k == 0u || n == k)
    {
        return 1u;
    }
    assert(k >= 1u && n >= 2u);

    // Index in the binomial number cache
    const unsigned int idx = (n - 2)*(n - 1) / 2 + (k - 1);

    if (idx >= binomial_numbers.size())
    {
        binomial_numbers.resize(idx + 1, 0u);
    }

    auto& binomial_number = binomial_numbers[idx];
    if (binomial_number > 0u)
    {
        return binomial_numbers[idx];
    }
    else
    {
        static const Rep MAX = overflowValue();

        Rep accumulator = 0u;
        for (unsigned int e = 0u; e <= nb_elts; e++)
        {
            const Rep partial = nb_alternatives_for_fixed_nb_of_partitions(e, nb_buckets - 1u);
            if (accumulator > MAX - partial)
            {
                accumulator = MAX;
                break;
            }
            accumulator += partial;
        }
        binomial_number = accumulator;
        return accumulator;
    }
}


WorkGrid::WorkGrid(const InputGrid& grid, Solver::Solutions* solutions, GridStats* stats, Solver::Observer observer) :
    OutputGrid(grid.cols.size(), grid.rows.size(), grid.name),
    rows(row_constraints_from(grid)),
    cols(column_constraints_from(grid)),
    saved_solutions(solutions),
    stats(stats),
    observer(std::move(observer)),
    nested_level(0u),
    binomial(new BinomialCoefficientsCache())
{
    assert(cols.size() == get_width());
    assert(rows.size() == get_height());
    assert(saved_solutions != nullptr);

    // Other initializations
    line_completed[Line::ROW].resize(get_height(), false);
    line_completed[Line::COL].resize(get_width(), false);
    line_to_be_reduced[Line::ROW].resize(get_height(), false);
    line_to_be_reduced[Line::COL].resize(get_width(), false);
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
    nested_level(nested_level),
    binomial(nullptr)               // only used on the first pass on the grid threfore on nested_level == 0
{
    assert(nested_level > 0u);

    line_completed[Line::ROW] = parent.line_completed[Line::ROW];
    line_completed[Line::COL] = parent.line_completed[Line::COL];

    line_to_be_reduced[Line::ROW] = parent.line_to_be_reduced[Line::ROW];
    line_to_be_reduced[Line::COL] = parent.line_to_be_reduced[Line::COL];

    nb_alternatives[Line::ROW] = parent.nb_alternatives[Line::ROW];
    nb_alternatives[Line::COL] = parent.nb_alternatives[Line::COL];

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
    if (t != Tile::UNKNOWN && get(x, y) == Tile::UNKNOWN)
    {
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
        assert(Tile::compatible(get(x, y), t));
        return false;
    }
}


bool WorkGrid::set_line(const Line& line)
{
    bool changed = false;
    const size_t index = line.get_index();
    const Line origin_line = get_line(line.get_type(), index);
    const auto width = static_cast<unsigned int>(get_width());
    const auto height = static_cast<unsigned int>(get_height());
    if (line.get_type() == Line::ROW)
    {
        assert(line.size() == width);
        for (unsigned int x = 0u; x < width; x++) { changed |= set_w_reduce_flag(x, index, line.at(x)); }
    }
    else
    {
        assert(line.get_type() == Line::COL);
        assert(line.size() == height);
        for (unsigned int y = 0u; y < height; y++) { changed |= set_w_reduce_flag(index, y, line.at(y)); }
    }
    if (observer && changed)
    {
        const Line delta = line_delta(origin_line, get_line(line.get_type(), index));
        observer(Solver::Event::DELTA_LINE, &delta, nested_level);
    }
    return changed;
}


// On the first pass of the grid, assume that the color of every tile in the line is unknown
// and compute the trivial reduction and number of alternatives
bool WorkGrid::single_line_initial_pass(Line::Type type, unsigned int index)
{
    const Constraint& constraint = type == Line::ROW ? rows.at(index) : cols.at(index);

    const auto line_size = static_cast<unsigned int>(type == Line::ROW ? get_width() : get_height());

    assert(binomial);
    Line reduced_line(type, index, line_size);  // All Tile::UNKNOWN

    // Compute the trivial reduction if it exists and the number of alternatives
    const auto pair = constraint.line_trivial_reduction(reduced_line, *binomial);

    const bool changed = pair.first;
    const auto nb_alt = pair.second;
    assert(nb_alt > 0u);
    nb_alternatives[type][index] = nb_alt;

    if (stats != nullptr) { stats->max_initial_nb_alternatives = std::max(stats->max_initial_nb_alternatives, nb_alt); }

    // If the reduced line is not compatible with the information already present in the grid
    // then the row and column constraints are contradictory.
    if (!get_line(type, index).compatible(reduced_line)) { throw PicrossGridCannotBeSolved(); }

    // Set line
    set_line(reduced_line);

    if (nb_alt == 1u)
    {
        // Line is completed
        line_completed[type][index] = true;
        line_to_be_reduced[type][index] = false;
    }
    else
    {
        // During a normal pass line_to_be_reduced is set to false after a line reduction has been performed.
        // Here since we are computing a trivial reduction assuming the initial line is completly unknown we
        // are ignoring tiles that are possibly already set. In such a case, we need to redo a reduction
        // on the next full grid pass.
        const Line new_line = get_line(type, index);
        if (reduced_line == new_line)
        {
            line_completed[type][index] = false;
            line_to_be_reduced[type][index] = false;
        }
        else
        {
            line_completed[type][index] = is_fully_defined(new_line);
            line_to_be_reduced[type][index] = !line_completed[type][index];
        }
    }

    return changed;
}

bool WorkGrid::single_line_pass(Line::Type type, unsigned int index)
{
    assert(line_completed[type][index] == false);
    assert(line_to_be_reduced[type][index] == true);

    if (stats != nullptr) { stats->nb_single_line_pass_calls++; }

    const Constraint& line_constraint = type == Line::ROW ? rows.at(index) : cols.at(index);
    const Line known_tiles = get_line(type, index);

    // Reduce all possible lines that match the data already present in the grid and the line constraint
    const auto reduction = line_constraint.reduce_and_count_alternatives(known_tiles, stats);
    const Line& reduced_line = reduction.first;
    const auto count = reduction.second;

    nb_alternatives[type][index] = count;

    if (stats != nullptr) { stats->max_nb_alternatives = std::max(stats->max_nb_alternatives, count); }

    // If the list of all lines is empty, it means the grid data is contradictory. Throw an exception.
    if (nb_alternatives[type][index] == 0) { throw PicrossGridCannotBeSolved(); }

    // If the list comprises of only one element, it means we solved that line
    if (nb_alternatives[type][index] == 1) { line_completed[type][index] = true; }

    // In any case, update the grid data with the reduced line resulting from list all_lines
    bool changed = set_line(reduced_line);

    // This line does not need to be reduced until one of the tiles is modified.
    line_to_be_reduced[type][index] = false;

    // return true if some tile was modified. False otherwise.
    return changed;
}


// Reduce all rows or all columns. Return false if no change was made on the grid.
bool WorkGrid::full_side_pass(Line::Type type, bool first_pass)
{
    const auto length = type == Line::ROW ? get_height() : get_width();
    bool changed = false;
    if (first_pass)
    {
        for (unsigned int x = 0u; x < length; x++)
        {
            changed |= single_line_initial_pass(type, x);
        }
    }
    else
    {
        for (unsigned int x = 0u; x < length; x++)
        {
            if (line_to_be_reduced[type][x]) { changed |= single_line_pass(type, x); }
        }
    }
    return changed;
}


// Reduce all columns and all rows. Return false if no change was made on the grid.
bool WorkGrid::full_grid_pass(bool first_pass)
{
    if (stats != nullptr) { stats->nb_full_grid_pass_calls++; }

    return full_side_pass(Line::COL, first_pass) | full_side_pass(Line::ROW, first_pass);
}


bool WorkGrid::all_lines_completed() const
{
    const bool all_rows = std::all_of(line_completed[Line::ROW].cbegin(), line_completed[Line::ROW].cend(), [](bool b) { return b; });
    const bool all_cols = std::all_of(line_completed[Line::COL].cbegin(), line_completed[Line::COL].cend(), [](bool b) { return b; });

    // The logical AND is important here: in the case an hypothesis is made on a row (resp. a column), it is marked as completed
    // but the constraints on the columns (resp. the rows) may not be all satisfied.
    return all_rows && all_cols;
}


bool WorkGrid::solve(unsigned int max_nb_solutions)
{
    try
    {
        bool grid_changed = full_grid_pass(nested_level == 0u);     // If nested_level == 0, this is the first pass on the grid
        bool grid_completed = all_lines_completed();

        while (!grid_completed && grid_changed)
        {
            // While the reduce method is making progress, call it!
            grid_changed = full_grid_pass();
            grid_completed = all_lines_completed();
        }

        // Are we done?
        if (grid_completed)
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
            Line::Type found_line_type = Line::ROW;
            unsigned int found_line_index = 0u;
            for (const auto& type : { Line::ROW, Line::COL })
            {
                for (unsigned int idx = 0u; idx < nb_alternatives[type].size(); idx++)
                {
                    const auto nb_alt = nb_alternatives[type][idx];
                    if (nb_alt >= 2u && (min_alt < 2u || nb_alt < min_alt))
                    {
                        min_alt = nb_alt;
                        found_line_type = type;
                        found_line_index = idx;
                    }
                }
            }

            if (min_alt == 0u)
            {
                throw std::logic_error("WorkGrid::solve: no min alternatives value, this should not happen!");
            }

            // Select the row or column with the minimal number of alternatives
            const Constraint& line_constraint = found_line_type == Line::ROW ? rows.at(found_line_index) : cols.at(found_line_index);
            const Line known_tiles = get_line(found_line_type, found_line_index);

            guess_list_of_all_alternatives = line_constraint.build_all_possible_lines(known_tiles);
            assert(guess_list_of_all_alternatives.size() == min_alt);

            // Guess!
            return guess(max_nb_solutions);
        }
    }
    catch(PicrossGridCannotBeSolved&)
    {
        // The puzzle couldn't be solved
        return false;
    }
}


bool WorkGrid::guess(unsigned int max_nb_solutions) const
{
    assert(guess_list_of_all_alternatives.size() > 0u);
    /* This function will test a range of alternatives for one particular line of the grid, each time
     * creating a new instance of the grid class on which the function WorkGrid::solve() is called.
     */

    if (stats != nullptr)
    {
        stats->guess_total_calls++;
        const auto nb_alternatives = static_cast<unsigned int>(guess_list_of_all_alternatives.size());
        stats->guess_total_alternatives += nb_alternatives;
        if (stats->guess_max_nb_alternatives_by_depth.size() < nested_level + 1)
            stats->guess_max_nb_alternatives_by_depth.resize(nested_level + 1);
        auto& max_nb_alternatives = stats->guess_max_nb_alternatives_by_depth[nested_level];
        max_nb_alternatives = std::max(max_nb_alternatives, nb_alternatives);
    }

    const Line::Type guess_line_type = guess_list_of_all_alternatives.front().get_type();
    const unsigned int guess_line_index = guess_list_of_all_alternatives.front().get_index();
    bool flag_solution_found = false;
    for (const Line& guess_line : guess_list_of_all_alternatives)
    {
        // Allocate a new work grid. Use the shallow copy.
        WorkGrid new_grid(*this, nested_level + 1);

        // Set one line in the new_grid according to the hypothesis we made. That line is then complete
        if (!new_grid.set_line(guess_line))
        {
            throw std::logic_error("WorkGrid::guess: no change in the new grid, will cause infinite loop.");
        }
        new_grid.line_completed[guess_line_type][guess_line_index] = true;
        new_grid.line_to_be_reduced[guess_line_type][guess_line_index] = false;
        new_grid.nb_alternatives[guess_line_type][guess_line_index] = 0u;

        if (max_nb_solutions > 0u && saved_solutions->size() >= max_nb_solutions)
            break;

        // Solve the new grid!
        flag_solution_found |= new_grid.solve(max_nb_solutions);
    }
    return flag_solution_found;
}


bool WorkGrid::valid_solution() const
{
    assert(is_solved());
    bool valid = true;
    for (unsigned int x = 0u; x < get_width(); x++)  { valid &= cols.at(x).compatible(get_line(Line::COL, x)); }
    for (unsigned int y = 0u; y < get_height(); y++) { valid &= rows.at(y).compatible(get_line(Line::ROW, y)); }
    return valid;
}


void WorkGrid::save_solution() const
{
    assert(valid_solution());
    if (stats != nullptr) { stats->nb_solutions++; }

    // Shallow copy of only the grid data
    saved_solutions->emplace_back(static_cast<const OutputGrid&>(*this));
}


Constraint::Constraint(Line::Type type, const InputGrid::Constraint& vect) :
    type(type),
    segs_of_ones(vect)
{
    if (segs_of_ones.size() == 0u)
    {
        min_line_size = 0u;
    }
    else
    {
        /* Include at least one zero between the sets of one */
        min_line_size = std::accumulate(segs_of_ones.cbegin(), segs_of_ones.cend(), 0u) + static_cast<unsigned int>(segs_of_ones.size()) - 1u;
    }
}


unsigned int Constraint::nb_filled_tiles() const
{
    return std::accumulate(segs_of_ones.cbegin(), segs_of_ones.cend(), 0u);
}


unsigned int Constraint::max_segment_size() const
{
    // Sets_of_ones vector must not be empty.
    if (segs_of_ones.empty()) { return 0u; }

    // Compute max
    return *max_element(segs_of_ones.cbegin(), segs_of_ones.cend());
}


void Constraint::print(std::ostream& ostream) const
{
    ostream << "Constraint on a " << str_line_type(type) << ": [ ";
    for (auto& it = segs_of_ones.begin(); it != segs_of_ones.end(); ++it)
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


/*
 * Given an uninitialized line compute the trivial reduction and the theoritical number of alternatives
 *
 *  - Set the line pass as argument to the trivial reduction, if there is one
 *  - Return a pair (changed, nb_alternatives)
 */
std::pair<bool, unsigned int> Constraint::line_trivial_reduction(Line& line, BinomialCoefficientsCache& binomial_cache) const
{
    assert(line.get_type() == type);
    assert(is_all_one_color(line, Tile::UNKNOWN));

    if (line.size() < min_line_size)
    {
        // This can happen if the user does not check the grid with check_grid_input()
        throw std::logic_error("Constraint::line_trivial_reduction: line.size() < min_line_size");
    }

    bool changed = false;
    unsigned int nb_alternatives = 0u;

    const unsigned int nb_zeros = line.size() - min_line_size;
    Line::Container& tiles = line.get_tiles();
    unsigned int line_idx = 0u;

    if (max_segment_size() == 0u)
    {
        // Blank line
        for (unsigned int c = 0u; c < line.size(); c++) { tiles[line_idx++] = Tile::ZERO; }

        assert(line_idx == line.size());
        assert(is_fully_defined(line));
        changed = true;
        nb_alternatives = 1;
    }
    else if (nb_zeros == 0u)
    {
        // The line is fully defined
        for (unsigned int seg_idx = 0u; seg_idx < segs_of_ones.size(); seg_idx++)
        {
            const auto seg_sz = segs_of_ones[seg_idx];
            const bool last = (seg_idx + 1u == segs_of_ones.size());
            for (unsigned int c = 0u; c < seg_sz; c++) { tiles[line_idx++] = Tile::ONE; }
            if (!last)                                 { tiles[line_idx++] = Tile::ZERO; }
        }

        assert(line_idx == line.size());
        assert(is_fully_defined(line));
        changed = true;
        nb_alternatives = 1;
    }
    else
    {
        if (max_segment_size() > nb_zeros)
        {
            // The reduction is straightforward in that case
            for (unsigned int seg_idx = 0u; seg_idx < segs_of_ones.size(); seg_idx++)
            {
                const auto seg_sz = segs_of_ones[seg_idx];
                const bool last = (seg_idx + 1u == segs_of_ones.size());
                for (unsigned int c = 0u; c < seg_sz; c++) { if (c >= nb_zeros) { tiles[line_idx] = Tile::ONE; }; line_idx++; }
                if (!last)                                 { line_idx++; /* would be Tile::ZERO */ }
            }

            assert(line_idx + nb_zeros == line.size());
            assert(!is_all_one_color(line, Tile::UNKNOWN));
            changed = true;
        }
        nb_alternatives = binomial_cache.nb_alternatives_for_fixed_nb_of_partitions(nb_zeros, static_cast<unsigned int>(segs_of_ones.size()) + 1u);
    }

    return std::make_pair(changed, nb_alternatives);
}


std::vector<Line> Constraint::build_all_possible_lines(const Line& known_tiles) const
{
    if (known_tiles.get_type() != type) { throw std::invalid_argument("Constraint::build_all_possible_lines: Wrong filter line type"); }
    const size_t index = known_tiles.get_index();

    // Number of zeros to add to the minimal size line.
    if (known_tiles.size() < min_line_size) { throw std::logic_error("Constraint::build_all_possible_lines: line_size < min_line_size"); }
    unsigned int nb_zeros = known_tiles.size() - min_line_size;

    std::vector<Line> result;
    Line new_line(known_tiles, Tile::UNKNOWN);
    Line::Container& new_tile_vect = new_line.get_tiles();

    if (segs_of_ones.size() == 0)
    {
        // Return a list with only one all-zero line
        unsigned int line_idx = 0u;
        for (unsigned int c = 0u; c < known_tiles.size(); c++)  { new_tile_vect[line_idx++] = Tile::ZERO; }

        // Filter against known_tiles
        if (new_line.compatible(known_tiles))
        {
            result.emplace_back(type, index, new_tile_vect);
        }
    }
    else if (segs_of_ones.size() == 1)
    {
        // Build the list of all possible lines with only one block of continuous filled tiles.
        // NB: in this case nb_zeros = size - segs_of_ones[0]
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            unsigned int line_idx = 0u;
            for (unsigned int c = 0u; c < n; c++)               { new_tile_vect[line_idx++] = Tile::ZERO; }
            for (unsigned int c = 0u; c < segs_of_ones[0]; c++) { new_tile_vect[line_idx++] = Tile::ONE;  }
            while(line_idx < known_tiles.size())                { new_tile_vect[line_idx++] = Tile::ZERO; }

            // Filter against known_tiles
            if (new_line.compatible(known_tiles))
            {
                result.emplace_back(type, index, new_tile_vect);
            }
        }
    }
    else
    {
        // For loop on the number of zeros before the first block of ones
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            unsigned int line_idx = 0u;
            for (unsigned int c = 0u; c < n; c++)               { new_tile_vect[line_idx++] = Tile::ZERO; }
            for (unsigned int c = 0u; c < segs_of_ones[0]; c++) { new_tile_vect[line_idx++] = Tile::ONE;  }
            new_tile_vect[line_idx++] = Tile::ZERO;

            // Filter against known_tiles
            if (new_line.compatible(known_tiles))
            {
                // If OK, then go on and recursively call this function to construct the remaining part of the line.
                std::vector<unsigned int> trim_sets_of_ones(segs_of_ones.begin() + 1, segs_of_ones.end());
                Constraint recursive_constraint(type, trim_sets_of_ones);

                std::vector<Tile::Type> end_known_vect(known_tiles.get_tiles().cbegin() + line_idx, known_tiles.get_tiles().cend());
                Line end_known_tiles(type, index, std::move(end_known_vect));

                std::vector<Line> recursive_list = recursive_constraint.build_all_possible_lines(end_known_tiles);

                // Finally, construct the return_list based on the contents of the recursive_list.
                for (const Line& line : recursive_list)
                {
                    std::copy(line.get_tiles().cbegin(), line.get_tiles().cend(), new_tile_vect.begin() + line_idx);
                    result.emplace_back(type, index, new_tile_vect);
                }
            }
            else
            {
                // The beginning of the line does not match the known_tiles. Do nothing.
            }
        }
        // Filtering is already done, no need to call add_and_filter_lines()
    }

    return result;
}


namespace
{
// Helper class to recursively build all the possible alternatives of a Line, given a constraint
class BuildLineAlternatives
{
public:
    BuildLineAlternatives(const InputGrid::Constraint& segs_of_ones, const Line& known_tiles)
        : segs_of_ones(segs_of_ones)
        , known_tiles(known_tiles)
        , reduced_line()
    {
    }

    unsigned int build_alternatives(const Line& alternative, unsigned int remaining_zeros, size_t line_idx = 0u, size_t constraint_idx = 0u)
    {
        assert(alternative.get_type() == known_tiles.get_type());
        assert(alternative.size() == known_tiles.size());

        unsigned int nb_alternatives = 0u;

        // If the last segment of ones was reached, pad end of line with zero, chack compatibility then reduce
        if (constraint_idx == segs_of_ones.size())
        {
            Line next_alternative(alternative);
            Line::Container& next_tiles = next_alternative.get_tiles();
            assert(next_tiles.size() - line_idx == remaining_zeros);
            for (unsigned int c = 0u; c < remaining_zeros; c++) { next_tiles[line_idx++] = Tile::ZERO; }

            nb_alternatives += filter_and_reduce(next_alternative);
        }
        // Else, fill in the next segment of ones, then call recursively
        else
        {
            const auto& nb_ones = segs_of_ones[constraint_idx];
            for (unsigned int pre_zeros = 0u; pre_zeros <= remaining_zeros; pre_zeros++)
            {
                Line next_alternative(alternative);
                auto next_line_idx = line_idx;
                Line::Container& next_tiles = next_alternative.get_tiles();
                for (unsigned int c = 0u; c < pre_zeros; c++) { next_tiles[next_line_idx++] = Tile::ZERO; }
                for (unsigned int c = 0u; c < nb_ones; c++)   { next_tiles[next_line_idx++] = Tile::ONE; }
                if (constraint_idx + 1 < segs_of_ones.size()) { next_tiles[next_line_idx++] = Tile::ZERO; }

                if (next_alternative.compatible(known_tiles))
                {
                    nb_alternatives += build_alternatives(next_alternative, remaining_zeros - pre_zeros, next_line_idx, constraint_idx + 1);
                }
            }
        }

        return nb_alternatives;
    };

    Line get_reduced_line()
    {
        if (reduced_line)
            return *reduced_line;
        else
            return Line(known_tiles, Tile::UNKNOWN);
    }

private:
    unsigned int filter_and_reduce(const Line& alternative)
    {
        assert(is_fully_defined(alternative));
        if (alternative.compatible(known_tiles))
        {
            if (!reduced_line)
            {
                reduced_line = std::make_unique<Line>(alternative);
            }
            else
            {
                reduced_line->reduce(alternative);
            }
            return 1u;
        }
        return 0u;
    }

private:
    const InputGrid::Constraint& segs_of_ones;
    const Line& known_tiles;
    std::unique_ptr<Line> reduced_line;
};

}  // namespace


std::pair<Line, unsigned int> Constraint::reduce_and_count_alternatives(const Line& known_tiles, GridStats * stats) const
{
    if (stats != nullptr) { stats->nb_reduce_and_count_alternatives_calls++; }

    if (known_tiles.get_type() != type) { throw std::invalid_argument("Constraint::reduce_and_count_alternatives: Wrong filter line type"); }
    const size_t index = known_tiles.get_index();

    // Number of zeros to add to the minimal size line.
    if (known_tiles.size() < min_line_size) { throw std::logic_error("Constraint::reduce_and_count_alternatives: line_size < min_line_size"); }
    unsigned int nb_zeros = known_tiles.size() - min_line_size;

    BuildLineAlternatives builder(segs_of_ones, known_tiles);
    Line seed_alternative(known_tiles, Tile::UNKNOWN);
    unsigned int nb_alternatives = builder.build_alternatives(seed_alternative, nb_zeros);

    return std::make_pair(builder.get_reduced_line(), nb_alternatives);
}

bool Constraint::compatible(const Line& line) const
{
    assert(is_fully_defined(line));
    const auto& tiles = line.get_tiles();
    InputGrid::Constraint line_segs_of_ones;
    unsigned int idx = 0u;
    while (idx < tiles.size())
    {
        while (idx < tiles.size() && tiles[idx] == Tile::ZERO) { idx++; }
        unsigned int seg_sz = 0u;
        while (idx < tiles.size() && tiles[idx] == Tile::ONE) { idx++; seg_sz++; }
        if (seg_sz > 0)
        {
            line_segs_of_ones.push_back(seg_sz);
        }
    }
    return segs_of_ones == line_segs_of_ones;
}


std::string get_grid_size(const InputGrid& grid)
{
    std::ostringstream oss;
    oss << grid.cols.size() << "x" << grid.rows.size();
    return oss.str();
}


std::pair<bool, std::string> check_grid_input(const InputGrid& grid)
{
    // Sanity check of the grid input
    //  -> height != 0 and width != 0
    //  -> same number of painted cells on rows and columns
    //  -> for each constraint min_line_size is smaller than the width or height of the row or column, respectively.

    const auto width  = static_cast<unsigned int>(grid.cols.size());
    const auto height = static_cast<unsigned int>(grid.rows.size());

    if (height == 0u)
    {
        std::ostringstream oss;
        oss << "Invalid height = " << height;
        return std::make_pair(false, oss.str());
    }
    if (width == 0u)
    {
        std::ostringstream oss;
        oss << "Invalid width = " << width;
        return std::make_pair(false, oss.str());
    }
    unsigned int nb_tiles_on_rows = 0u;
    for (auto& c = grid.rows.begin(); c != grid.rows.end(); c++)
    {
        Constraint row(Line::ROW, *c);
        nb_tiles_on_rows += row.nb_filled_tiles();
        if (row.get_min_line_size() > width)
        {
            std::ostringstream oss;
            oss << "Width = " << width << " of the grid is too small for constraint: " << row;
            return std::make_pair(false, oss.str());
        }
    }
    unsigned int nb_tiles_on_cols = 0u;
    for (auto& c = grid.cols.begin(); c != grid.cols.end(); c++)
    {
        Constraint col(Line::COL, *c);
        nb_tiles_on_cols += col.nb_filled_tiles();
        if (col.get_min_line_size() > height)
        {
            std::ostringstream oss;
            oss << "Height = " << height << " of the grid is too small for constraint: " << col;
            return std::make_pair(false, oss.str());
        }
    }
    if (nb_tiles_on_rows != nb_tiles_on_cols)
    {
        std::ostringstream oss;
        oss << "Number of filled tiles on rows (" << nb_tiles_on_rows << ") and columns (" <<  nb_tiles_on_cols << ") do not match";
        return std::make_pair(false, oss.str());
    }
    return std::make_pair(true, std::string());
}


std::ostream& operator<<(std::ostream& ostream, const GridStats& stats)
{
    ostream << "  Number of solutions found: " << stats.nb_solutions;
    if (stats.max_nb_solutions > 0)
    {
        ostream << "/" << stats.max_nb_solutions;
    }
    ostream << std::endl;
    ostream << "  Max branching depth: " << stats.max_nested_level << std::endl;
    if (stats.max_nested_level > 0u)
    {
        ostream << "    > The solving of the grid required an hypothesis on " << stats.guess_total_calls << " row(s) or column(s)" << std::endl;
        ostream << "    > Total nunmber of alternatives being tested: " << stats.guess_total_alternatives << std::endl;
        ostream << "    > Max nunmber of alternatives by depth:";
        for (const auto& max_alternatives : stats.guess_max_nb_alternatives_by_depth)
        {
            ostream << " " << max_alternatives;
        }
        ostream << std::endl;
    }
    ostream << "  Max initial number of alternatives on a line: " << stats.max_initial_nb_alternatives;
    if (stats.max_initial_nb_alternatives == BinomialCoefficientsCache::overflowValue())
    {
        ostream << " (overflow!)";
    }
    ostream << std::endl;
    ostream << "  Max number of alternatives during a line reduce: " << stats.max_nb_alternatives << std::endl;
    ostream << "  " << stats.nb_full_grid_pass_calls << " calls to full_grid_pass()" << std::endl;
    ostream << "  " << stats.nb_single_line_pass_calls << " calls to single_line_pass()" << std::endl;
    ostream << "  " << stats.nb_reduce_and_count_alternatives_calls << " calls to reduce_and_count_alternatives()" << std::endl;
    ostream << "  " << stats.nb_observer_callback_calls << " calls to observer callback." << std::endl;

    return ostream;
}


Solver::Solutions RefSolver::solve(const InputGrid& grid_input, unsigned int max_nb_solutions) const
{
    Solutions solutions;

    if (stats != nullptr)
    {
        /* Reset stats */
        std::swap(*stats, GridStats());

        stats->max_nb_solutions = max_nb_solutions;
    }

    Observer observer_wrapper;
    if (observer)
    {
        observer_wrapper = [this](Solver::Event event, const Line* delta, unsigned int depth)
        {
            if (this->stats != nullptr)
            {
                this->stats->nb_observer_callback_calls++;
            }
            this->observer(event, delta, depth);
        };
    }

    const bool success = WorkGrid(grid_input, &solutions, stats, std::move(observer_wrapper)).solve(max_nb_solutions);

    assert(success == (solutions.size() > 0u));
    return solutions;
}


void RefSolver::set_observer(Observer observer)
{
    this->observer = std::move(observer);
}


void RefSolver::set_stats(GridStats& stats)
{
    this->stats = &stats;
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


std::pair<bool, std::string> validate_input_grid(const Solver& solver, const InputGrid& grid_input)
{
    bool check;
    std::string check_msg;
    std::tie(check, check_msg) = picross::check_grid_input(grid_input);

    if (!check)
    {
        return std::make_pair(false, check_msg);
    }

    const auto solutions = solver.solve(grid_input, 2);

    if (solutions.empty())
    {
        return std::make_pair(false, "No solution could be found");
    }
    else if (solutions.size() > 1)
    {
        return std::make_pair(false, "The grid does not have a unique solution");
    }

    return std::make_pair(true, std::string());
}


std::unique_ptr<Solver> get_ref_solver()
{
    return std::make_unique<RefSolver>();
}

} // namespace picross

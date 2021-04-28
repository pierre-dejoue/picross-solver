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


/* Add (with Tile::add) a filter line to each line in a list. If the addition fails,
 * the offending line is discarded from the list
 */
void add_and_filter_lines(std::vector<Line>& lines, const Line& known_tiles, GridStats* stats)
{
    // Stats
    if (stats != nullptr)
    {
         const auto list_size = static_cast<unsigned int>(lines.size());
         stats->nb_add_and_filter_calls++;
         stats->total_lines_added_and_filtered += list_size;
         if (list_size > stats->max_add_and_filter_list_size) { stats->max_add_and_filter_list_size = list_size; }
    }

    auto it = lines.begin();
    while(it != lines.end())
    {
        const bool compatible = it->add(known_tiles);
        it = compatible ? std::next(it) : lines.erase(it);
    }
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


WorkGrid::WorkGrid(const InputGrid& grid, Solver::Solutions* solutions, GridStats* stats, Solver::Observer observer) :
    OutputGrid(grid.cols.size(), grid.rows.size(), grid.name),
    rows(row_constraints_from(grid)),
    cols(column_constraints_from(grid)),
    saved_solutions(solutions),
    stats(stats),
    observer(std::move(observer)),
    nested_level(0u),
    cache_binomial_numbers()
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
    nested_level(nested_level),
    cache_binomial_numbers()
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
    const Line origin_line = get_line(line.get_type(), index);
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
    if (observer && changed)
    {
        const Line delta = line_delta(origin_line, line);
        observer(Solver::Event::DELTA_LINE, &delta, nested_level);
    }
    return changed;
}


bool WorkGrid::single_line_pass(Line::Type type, unsigned int index)
{
    assert(line_completed[type][index] == false);
    assert(line_to_be_reduced[type][index] == true);

    const Constraint& line_constraint = type == Line::ROW ? rows.at(index) : cols.at(index);

    // Stats
    if (stats != nullptr) { stats->nb_single_line_pass_calls++;  }

    // If the color of every tile in the line is unknown, there is a simple way to determine
    // if the reduce operation is relevant: max block size must be greater than line_size - min_line_size.
    const Line known_tiles = get_line(type, index);

    if (known_tiles.size() < line_constraint.get_min_line_size())
    {
        throw std::logic_error("WorkGrid::single_line_pass: line_size < min_line_size");
    }
    const unsigned int nb_zeros = known_tiles.size() - line_constraint.get_min_line_size();

    if (is_all_one_color(known_tiles, Tile::UNKNOWN))
    {
        if (0u < line_constraint.max_segment_size() && line_constraint.max_segment_size() <= nb_zeros)
        {
            // Cannot reduce this line until one of the tiles is modified.
            line_to_be_reduced[type][index] = false;

            // We shall compute the number of alternatives anyway (in case we have to make an hypothesis on the grid)
            nb_alternatives[type][index] = nb_alternatives_for_fixed_nb_of_partitions(nb_zeros, static_cast<unsigned int>(line_constraint.nb_segments()) + 1u);

            // No change made on this line
            return false;
        }
        else
        {
            // TODO The reduction is straightforward in that case
        }
    }

    // Compute all possible lines that match the data already present in the grid and the line constraints
    const auto reduction = line_constraint.reduce_and_count_alternatives(known_tiles, stats);
    const Line& reduced_line = reduction.first;
    const auto count = reduction.second;

    nb_alternatives[type][index] = count;

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
    const bool all_cols = std::all_of(line_completed[Line::COL].cbegin(), line_completed[Line::COL].cend(), [](bool b) { return b; });

    // The logical and is important here: in the case of an hypothesis is made on a row (resp. a column), it is marked as completed
    // but the constraints on the columns (resp. the rows) may not be all OK.
    return all_rows && all_cols;
}


bool WorkGrid::solve()
{
    try
    {
        bool grid_changed = false;
        bool grid_completed = false;
        do
        {
            // While the reduce method is making progress, call it!
            grid_changed = full_grid_pass();
            grid_completed = all_lines_completed();

        } while (!grid_completed && grid_changed);

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

            guess_list_of_all_alternatives = line_constraint.build_all_possible_lines(known_tiles, stats);
            assert(guess_list_of_all_alternatives.size() == min_alt);

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

        // Solve the new grid!
        flag_solution_found |= new_grid.solve();
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

    // Shallow copy of only the grid data
    saved_solutions->emplace_back(static_cast<const OutputGrid&>(*this));
}

/* Returns the number of ways to divide nb_elts into nb_buckets.
 *
 * Equal to the binomial coefficient (n k) with n = nb_elts + nb_buckets - 1  and k = nb_buckets - 1
 *
 * Returns std::numeric_limits<unsigned int>::max() in case of overflow (which happens rapidly)
 */
unsigned int WorkGrid::nb_alternatives_for_fixed_nb_of_partitions(unsigned int nb_elts, unsigned int nb_buckets)
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

    if (idx >= cache_binomial_numbers.size())
    {
        cache_binomial_numbers.resize(idx + 1, 0u);
    }

    auto& binomial_number = cache_binomial_numbers[idx];
    if (binomial_number > 0u)
    {
        return cache_binomial_numbers[idx];
    }
    else
    {
        static const unsigned int MAX = std::numeric_limits<unsigned int>::max();

        unsigned int accumulator = 0u;
        for (unsigned int c = 0u; c <= nb_elts; c++)
        {
            const unsigned int partial = nb_alternatives_for_fixed_nb_of_partitions(c, nb_buckets - 1u);
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


std::vector<Line> Constraint::build_all_possible_lines(const Line& known_tiles, GridStats* stats) const
{
    if (known_tiles.get_type() != type) { throw std::invalid_argument("Constraint::build_all_possible_lines: Wrong filter line type"); }
    const size_t index = known_tiles.get_index();

    // Number of zeros to add to the minimal size line.
    if (known_tiles.size() < min_line_size) { throw std::logic_error("Constraint::build_all_possible_lines: line_size < min_line_size"); }
    unsigned int nb_zeros = known_tiles.size() - min_line_size;

    std::vector<Line> result;
    std::vector<Tile::Type> new_tile_vect(known_tiles.size(), Tile::ZERO);

    if (segs_of_ones.size() == 0)
    {
        // Return a list with only one all-zero line
        result.emplace_back(type, index, new_tile_vect);

        // Filter the return_list against input line
        add_and_filter_lines(result, known_tiles, stats);
    }
    else if (segs_of_ones.size() == 1)
    {
        // Build the list of all possible lines with only one block of continuous filled tiles.
        // NB: in this case nb_zeros = size - segs_of_ones[0]
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            for (unsigned int idx = 0u; idx < n; idx++)                                      { new_tile_vect.at(idx)   = Tile::ZERO; }
            for (unsigned int idx = 0u; idx < segs_of_ones[0]; idx++)                        { new_tile_vect.at(n+idx) = Tile::ONE;  }
            for (unsigned int idx = n + segs_of_ones[0]; idx < known_tiles.size(); idx++)    { new_tile_vect.at(idx) = Tile::ZERO; }
            result.emplace_back(type, index, new_tile_vect);
        }

        // Filter the return_list against known_tiles
        if (!is_all_one_color(known_tiles, Tile::UNKNOWN)) { add_and_filter_lines(result, known_tiles, stats); }
    }
    else
    {
        // For loop on the number of zeros before the first block of ones
        for (unsigned int n = 0u; n <= nb_zeros; n++)
        {
            for (unsigned int idx = 0u; idx < n; idx++)                  { new_tile_vect.at(idx)   = Tile::ZERO; }
            for (unsigned int idx = 0u; idx < segs_of_ones[0]; idx++)    { new_tile_vect.at(n+idx) = Tile::ONE;  }
            const unsigned int begin_size = n + segs_of_ones[0] + 1u;
            new_tile_vect.at(begin_size - 1u) = Tile::ZERO;

            // Add the start of the line (first block of ones) and the beginning of the filter line
            std::vector<Tile::Type> begin_vect(new_tile_vect.begin(), new_tile_vect.begin() + begin_size);
            std::vector<Tile::Type> begin_filter_vect(known_tiles.get_tiles().cbegin(), known_tiles.get_tiles().cbegin() + begin_size);

            Line begin_line(type, index, begin_vect);
            Line begin_filter(type, index, begin_filter_vect);
            if (begin_line.add(begin_filter))
            {
                // If OK, then go on and recursively call this function to construct the remaining part of the line.
                std::vector<unsigned int> trim_sets_of_ones(segs_of_ones.begin() + 1, segs_of_ones.end());
                Constraint recursive_constraint(type, trim_sets_of_ones);

                std::vector<Tile::Type> end_filter_vect(known_tiles.get_tiles().cbegin() + begin_size, known_tiles.get_tiles().cend());
                Line end_filter(type, index, end_filter_vect);

                std::vector<Line> recursive_list = recursive_constraint.build_all_possible_lines(end_filter, stats);

                // Finally, construct the return_list based on the contents of the recursive_list.
                for (const Line& line : recursive_list)
                {
                    std::copy(line.get_tiles().cbegin(), line.get_tiles().cend(), new_tile_vect.begin() + begin_size);
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
    ostream << "  Max branching depth: " << stats.max_nested_level << std::endl;
    if (stats.max_nested_level > 0u)
    {
        ostream << "    > The solving of the grid required an hypothesis on " << stats.guess_total_calls << " row(s) or column(s)." << std::endl;
        ostream << "    > Total nunmber of alternatives being tested: " << stats.guess_total_alternatives << std::endl;
        ostream << "    > Max nunmber of alternatives by depth:";
        for (const auto& max_alternatives : stats.guess_max_nb_alternatives_by_depth)
        {
            ostream << " " << max_alternatives;
        }
        ostream << std::endl;
    }
    ostream << "  " << stats.nb_full_grid_pass_calls << " calls to full_grid_pass()." << std::endl;
    ostream << "  " << stats.nb_single_line_pass_calls << " calls to single_line_pass()." << std::endl;
    ostream << "  " << stats.nb_add_and_filter_calls << " calls to add_and_filter_lines(). Max list size/total nb of lines being added and filtered: " << stats.max_add_and_filter_list_size << "/" << stats.total_lines_added_and_filtered << std::endl;
    ostream << "  " << stats.nb_observer_callback_calls << " calls to observer callback." << std::endl;

    return ostream;
}


Solver::Solutions RefSolver::solve(const InputGrid& grid_input) const
{
    Solutions solutions;

    if (stats != nullptr)
    {
        /* Reset stats */
        std::swap(*stats, GridStats());
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

    const bool success = WorkGrid(grid_input, &solutions, stats, std::move(observer_wrapper)).solve();

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


std::unique_ptr<Solver> get_ref_solver()
{
    return std::make_unique<RefSolver>();
}

} // namespace picross

/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - Definition of the output grid
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <vector>


namespace picross
{

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
std::ostream& operator<<(std::ostream& out, const Line& line);
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

std::ostream& operator<<(std::ostream& out, const LineId& line_id);


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

    friend std::ostream& operator<<(std::ostream& out, const OutputGrid& grid);
private:
    std::unique_ptr<Grid> p_grid;
};


/*
 * Return the grid size as a string "WxH" with W the width and H the height
 */
std::string str_output_grid_size(const OutputGrid& grid);

} // namespace picross

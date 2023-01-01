#pragma once

#include <picross/picross.h>

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>


namespace picross
{
class LineSpan;

class Grid
{
public:
    Grid(std::size_t width, std::size_t height, const std::string& name = std::string{});

    Grid(const Grid&) = default;
    Grid(Grid&&) noexcept = default;
    Grid& operator=(const Grid&);
    Grid& operator=(Grid&&) noexcept;

    std::size_t width() const { return m_width; }
    std::size_t height() const { return m_height; }

    const std::string& name() const { return m_name; }

    LineSpan get_line(Line::Type type, Line::Index index) const;
    Tile get(Line::Index x, Line::Index y) const;

    // set() will force whatever tile value is passed as argument
    // update() will change the tile value from unknwon to the value passed as argument, otherwise let it unchanged
    // Return true if the tile value in the grid has changed
    bool set(Line::Index x, Line::Index y, Tile val);
    bool update(Line::Index x, Line::Index y, Tile val);

    void reset();

    bool is_solved() const;

    std::size_t hash() const;

    friend bool operator==(const Grid& lhs, const Grid& rhs);
    friend bool operator!=(const Grid& lhs, const Grid& rhs);

private:
    const std::size_t       m_width;
    const std::size_t       m_height;
    const std::string       m_name;
    std::vector<Tile>       m_row_major;
    std::vector<Tile>       m_col_major;
};

std::ostream& operator<<(std::ostream& ostream, const Grid& grid);

} // namespace picross

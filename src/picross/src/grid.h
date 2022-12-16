#pragma once

#include <picross/picross.h>

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>


namespace picross
{

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

    Tile get(std::size_t x, std::size_t y) const;

    template <Line::Type type>
    Line get_line(std::size_t index) const;

    Line get_line(Line::Type type, std::size_t index) const;

    bool set(std::size_t x, std::size_t y, Tile val);
    void reset();

    bool is_solved() const;

    std::size_t hash() const;

    friend bool operator==(const Grid& lhs, const Grid& rhs);
    friend bool operator!=(const Grid& lhs, const Grid& rhs);

private:
    const std::size_t       m_width;
    const std::size_t       m_height;
    const std::string       m_name;
    std::vector<Tile>       m_grid;
};

std::ostream& operator<<(std::ostream& ostream, const Grid& grid);

} // namespace picross

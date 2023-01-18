#include <utils/test_helpers.h>

#include <cassert>
#include <exception>

namespace picross
{
OutputGrid build_output_grid_from(std::size_t width, std::size_t height, const std::vector<int>& tiles, std::string_view name)
{
    assert(tiles.size() == width * height);
    OutputGrid result(width, height, std::string{name});

    auto it = tiles.begin();
    for (std::size_t y = 0u; y < height; y++)
    {
        for (std::size_t x = 0u; x < width; x++)
        {
            assert(it != tiles.end());
            if (*it >= 0)
                result.set_tile(x, y, *it ? Tile::FILLED : Tile::EMPTY);
            ++it;
        }
    }
    return result;
}


OutputGrid build_output_grid_from(std::size_t width, std::size_t height, std::string_view tiles, std::string_view name)
{
    std::vector<int> tiles_vect;
    tiles_vect.reserve(width * height);
    for (const auto c: tiles)
    {
        if (c == '?')
            tiles_vect.push_back(-1);           // UNKNOWN
        else if (c == '0' || c == '.')
            tiles_vect.push_back(0);            // EMPTY
        else if (c == '1' || c == '#')
            tiles_vect.push_back(1);            // FILLED
        // Ignore all other characters
    }
    assert(tiles_vect.size() == (width * height));
    return build_output_grid_from(width, height, tiles_vect, name);
}


Line build_line_from(std::string_view tiles, Line::Type type, unsigned int index)
{
    std::vector<Tile> tiles_vect;
    tiles_vect.reserve(tiles.size());
    for (const auto c: tiles)
    {
        if (c == '.')
            tiles_vect.push_back(Tile::EMPTY);
        else if (c == '#')
            tiles_vect.push_back(Tile::FILLED);
        else if (c == '?')
            tiles_vect.push_back(Tile::UNKNOWN);
        else
            throw std::invalid_argument("Invalid tile character: " + c);
    }
    assert(tiles_vect.size() == tiles.size());
    return Line(type, index, std::move(tiles_vect));
}


Line build_line_from(char tile, std::size_t count, Line::Type type, unsigned int index)
{
    return build_line_from(std::string(count, tile), type, index);
}


std::size_t OutputGridHasher::operator()(const OutputGrid& grid) const noexcept
{
    return grid.hash();
}
} // namespace picross

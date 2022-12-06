#include <utils/test_helpers.h>

#include <cassert>

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
            result.set(x, y, *it ? Tile::FILLED : Tile::EMPTY);
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
        if (c == '0' || c == '.')
            tiles_vect.push_back(0);
        if (c == '1' || c == '#')
            tiles_vect.push_back(1);
    }
    assert(tiles_vect.size() == (width * height));
    return build_output_grid_from(width, height, tiles_vect, name);
}


std::size_t OutputGridHasher::operator()(const OutputGrid& grid) const noexcept
{
    return grid.hash();
}
} // namespace picross

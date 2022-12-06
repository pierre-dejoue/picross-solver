#include <utils/test_helpers.h>

#include <cassert>

picross::OutputGrid build_output_grid_from(std::size_t width, std::size_t height, const std::vector<int>& tiles)
{
    assert(tiles.size() == width * height);
    picross::OutputGrid result(width, height, "Test");

    auto it = tiles.begin();
    for (std::size_t y = 0u; y < height; y++)
    {
        for (std::size_t x = 0u; x < width; x++)
        {
            assert(it != tiles.end());
            result.set(x, y, *it ? picross::Tile::FILLED : picross::Tile::EMPTY);
            ++it;
        }
    }
    return result;
}

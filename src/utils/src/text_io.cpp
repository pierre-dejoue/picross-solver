#include <utils/strings.h>
#include <utils/text_io.h>

#include <algorithm>
#include <cassert>
#include <exception>
#include <fstream>
#include <sstream>


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
    if (tiles_vect.size() != (width * height))
    {
        std::stringstream msg;
        msg << "The number of tiles (" << tiles_vect.size() << ") does not match the requested grid size (" << width << "x" << height << ")";
        throw std::logic_error(msg.str());
    }
    return build_output_grid_from(width, height, tiles_vect, name);
}

OutputGrid build_output_grid_from(std::string_view tiles, std::string_view name)
{
    bool puzzle_line = false;
    std::size_t current_width = 0;
    std::size_t width = 0;
    std::size_t height = 0;
    for (const auto c: tiles)
    {
        if (c == '?' || c == '.' || c == '#' || c == '0' || c == '1')
        {
            current_width++;
            puzzle_line = true;
        }
        else if (c == '\n' && puzzle_line)
        {
            width = std::max(width, current_width);
            current_width = 0;
            height++;
            puzzle_line = false;
        }
    }
    if (puzzle_line)
    {
        width = std::max(width, current_width);
        height++;
    }
    return build_output_grid_from(width, height, tiles, name);
}

OutputGrid io::parse_output_grid_from_file(std::string_view filepath, const io::ErrorHandler& error_handler) noexcept
{
    try
    {
        std::vector<char> file_content;
        file_content.reserve(1024u);
        std::ifstream inputstream(filepath.data());
        if (inputstream.is_open())
        {
            // Start parsing
            while (inputstream.good())
            {
                char c;
                if (!inputstream.get(c).eof())
                    file_content.push_back(c);
            }
            file_content.push_back('\0');
            return build_output_grid_from(std::string_view(file_content.data()), file_name(filepath));
        }
        else
        {
            std::ostringstream oss;
            oss << "Cannot open file " << filepath;
            error_handler(oss.str(), 1);
        }
    }
    catch (std::logic_error& l)
    {
        std::ostringstream oss;
        oss << "Parsing error: " << l.what();
        error_handler(oss.str(), 2);
    }
    catch (std::exception& e)
    {
        std::ostringstream oss;
        oss << "Unhandled exception during file parsing: " << e.what();
        error_handler(oss.str(), 3);
    }
    return OutputGrid(0, 0, "Invalid");
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

} // namespace picross

/*******************************************************************************
 * PICROSS SOLVER
 *
 *   This file implements the file IO of the Picross solver
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross_io.h>

#include <stdutils/io.h>
#include <stdutils/macros.h>
#include <stdutils/string.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>

namespace picross {

IOGrid::IOGrid(const InputGrid& input_grid, const std::optional<OutputGrid>& goal)
    : m_input_grid(input_grid)
    , m_goal(goal)
{}

IOGrid::IOGrid(InputGrid&& input_grid, std::optional<OutputGrid>&& goal) noexcept
    : m_input_grid(std::move(input_grid))
    , m_goal(std::move(goal))
{}

namespace io {

std::string str_error_code(ErrorCodeT code)
{
    switch (code)
    {
        case ErrorCode::PARSING_ERROR:
            return "PARSING_ERROR";

        case ErrorCode::FILE_ERROR:
            return "FILE_ERROR";

        case ErrorCode::EXCEPTION:
            return "EXCEPTION";

        case ErrorCode::WARNING:
            return "WARNING";

        default:
        {
            std::ostringstream oss;
            oss << "ERROR_CODE(" << code << ")";
            return oss.str();
        }
    }
}

namespace {

struct FileFormat
{
    struct Native {};
    struct Nin {};
    struct Non {};
};

struct GridComponents
{
    InputGrid::Constraints      m_rows;
    InputGrid::Constraints      m_cols;
    std::string                 m_name;
    InputGrid::Metadata         m_metadata;
    std::optional<OutputGrid>   m_goal;
};

using ParserErrorHandler = std::function<void(std::string_view)>;

template <typename F>
class FileParser;

/******************************************************************************
 * Parser for the NATIVE file format
 ******************************************************************************/
template<>
class FileParser<FileFormat::Native>
{
private:
    enum class ParsingState
    {
        FILE_START,
        GRID_START,
        ROW_SECTION,
        COLUMN_SECTION
    };

public:
    FileParser() : parsing_state(ParsingState::FILE_START)
    {
    }

    void parse_line(const std::string& line_to_parse, std::vector<GridComponents>& grids, const ParserErrorHandler& error_handler)
    {
        std::istringstream iss(line_to_parse);
        std::string token;

        // Copy the first word in 'token' (trailing whitespaces are skipped)
        iss >> token;

        if (token == "GRID")
        {
            parsing_state = ParsingState::GRID_START;
            grids.emplace_back();
            if (line_to_parse.size() > 5u)
            {
                grids.back().m_name = line_to_parse.substr(5u);
            }
        }
        else if (token == "ROWS")
        {
            if (parsing_state != ParsingState::FILE_START)
            {
                parsing_state = ParsingState::ROW_SECTION;
            }
            else
            {
                error_decorator(error_handler, "Grid not started");
            }
        }
        else if (token == "COLUMNS")
        {
            if (parsing_state != ParsingState::FILE_START)
            {
                parsing_state = ParsingState::COLUMN_SECTION;
            }
            else
            {
                error_decorator(error_handler, "Grid not started");
            }
        }
        else if (token == "[")
        {
            if (parsing_state == ParsingState::ROW_SECTION)
            {
                picross::InputGrid::Constraint new_row;
                unsigned int n;
                while (iss >> n) { new_row.push_back(n); }
                grids.back().m_rows.push_back(std::move(new_row));
            }
            else if (parsing_state == ParsingState::COLUMN_SECTION)
            {
                picross::InputGrid::Constraint new_col;
                unsigned int n;
                while (iss >> n) { new_col.push_back(n); }
                grids.back().m_cols.push_back(std::move(new_col));
            }
            else
            {
                error_decorator(error_handler, "Unexpected token " + token);
            }
        }
        else if (token == "#")
        {
            // Comment lines are ignored
        }
        else
        {
            assert(!token.empty());        // Blank lines are already filtered out
            error_decorator(error_handler, "Invalid token " + token);
        }
    }

private:
    const char* parsing_state_str()
    {
        switch (parsing_state)
        {
        case ParsingState::FILE_START:
            return "FILE_START";
            break;
        case ParsingState::GRID_START:
            return "GRID_START";
            break;
        case ParsingState::ROW_SECTION:
            return "ROW_SECTION";
            break;
        case ParsingState::COLUMN_SECTION:
            return "COLUMN_SECTION";
            break;
        default:
            assert(0);
        }
        return "UNKNOWN";
    }

    void error_decorator(const ParserErrorHandler& error_handler, const std::string_view& msg)
    {
        std::ostringstream oss;
        oss << msg << " (parsing_state = " << parsing_state_str() << ")";
        error_handler(oss.str());
    }
private:
    ParsingState parsing_state;
};


/******************************************************************************
 * Parser for the NIN file format
 ******************************************************************************/
template<>
class FileParser<FileFormat::Nin>
{
private:
    enum class ParsingState
    {
        GRID_SIZE,
        ROW_SECTION,
        COLUMN_SECTION,
        DONE
    };

public:
    FileParser()
        : parsing_state(ParsingState::GRID_SIZE)
        , nb_rows(0)
        , nb_cols(0)
    {
    }

    void parse_line(const std::string& line_to_parse, std::vector<GridComponents>& grids, const ParserErrorHandler& error_handler)
    {
        UNUSED(error_handler);

        // Blank lines are already filtered out

        // Ignore comments
        if (line_to_parse[0] == '#')
            return;

        std::istringstream iss(line_to_parse);
        switch(parsing_state)
        {
        case ParsingState::GRID_SIZE:
            iss >> nb_cols;
            iss >> nb_rows;
            grids.emplace_back();
            grids.back().m_name = "No name";
            parsing_state = ParsingState::ROW_SECTION;
            break;

        case ParsingState::ROW_SECTION:
        {
            picross::InputGrid::Constraint new_row;
            unsigned int n;
            while (iss >> n) { new_row.push_back(n); }
            grids.back().m_rows.push_back(std::move(new_row));
            if (--nb_rows == 0)
                parsing_state = ParsingState::COLUMN_SECTION;
            break;
        }

        case ParsingState::COLUMN_SECTION:
        {
            picross::InputGrid::Constraint new_col;
            unsigned int n;
            while (iss >> n) { new_col.push_back(n); }
            grids.back().m_cols.push_back(std::move(new_col));
            if (--nb_cols == 0)
                parsing_state = ParsingState::DONE;
            break;
        }

        case ParsingState::DONE:
            // Ignore line
            break;

        default:
            assert(0);
        }
    }

private:
    std::string_view parsing_state_str()
    {
        switch (parsing_state)
        {
        case ParsingState::GRID_SIZE:
            return "GRID_SIZE";
            break;
        case ParsingState::ROW_SECTION:
            return "ROW_SECTION";
            break;
        case ParsingState::COLUMN_SECTION:
            return "COLUMN_SECTION";
            break;
        case ParsingState::DONE:
            return "DONE";
            break;
        default:
            assert(0);
        }
        return "UNKNOWN";
    }

    void error_decorator(const ParserErrorHandler& error_handler, const std::string_view& msg)
    {
        std::ostringstream oss;
        oss << msg << " (parsing_state = " << parsing_state_str() << ")";
        error_handler(oss.str());
    }
private:
    ParsingState parsing_state;
    unsigned int nb_rows;
    unsigned int nb_cols;
};


/******************************************************************************
 * Parser for the NON file format
 ******************************************************************************/
template<>
class FileParser<FileFormat::Non>
{
private:
    enum class ParsingState
    {
        Default,
        Rows,
        Columns
    };

public:
    FileParser()
      : parsing_state(ParsingState::Default)
      , width(0u)
      , height(0u)
    {

    }

    void parse_line(const std::string& line_to_parse, std::vector<GridComponents>& grids, const ParserErrorHandler& error_handler)
    {
        std::istringstream iss(line_to_parse);
        std::string token;

        // This file format can only define a single grid
        if (grids.empty())
        {
            grids.emplace_back();
        }
        auto& grid = grids.back();

        if (parsing_state == ParsingState::Rows)
        {
            if (grid.m_rows.size() == height)
            {
                parsing_state = ParsingState::Default;
            }
            else
            {
                const bool status = parse_constraint_line(iss, grid.m_rows.emplace_back());
                if (!status) { error_decorator(error_handler, "Invalid constraint"); }
            }

        }
        if (parsing_state == ParsingState::Columns)
        {
            if (grid.m_cols.size() == width)
            {
                parsing_state = ParsingState::Default;
            }
            else
            {
                const bool status = parse_constraint_line(iss, grid.m_cols.emplace_back());
                if (!status) { error_decorator(error_handler, "Invalid constraint"); }
            }
        }
        if (parsing_state == ParsingState::Default)
        {
            // Copy the first word in 'token'
            iss >> token;
            assert(!token.empty());         // Blank lines are already filtered out

            if (token == "title")
            {
                std::stringbuf remaining;
                iss >> &remaining;
                grid.m_name = extract_text_in_quotes_or_ltrim(remaining.str());
            }
            else if (token == "width")
            {
                iss >> width;
            }
            else if (token == "height")
            {
                iss >> height;
            }
            else if (token == "rows")
            {
                if (!grid.m_rows.empty())
                {
                    error_decorator(error_handler, "rows are already defined", token);
                }
                else if (height == 0)
                {
                    error_decorator(error_handler, "height wasn't defined", token);
                }
                else
                {
                    parsing_state = ParsingState::Rows;
                }
            }
            else if (token == "columns")
            {
                if (!grid.m_cols.empty())
                {
                    error_decorator(error_handler, "columns are already defined", token);
                }
                else if (width == 0)
                {
                    error_decorator(error_handler, "width wasn't defined", token);
                }
                else
                {
                    parsing_state = ParsingState::Columns;
                }
            }
            else if (token == "goal")
            {
                if (width == 0 || height == 0)
                {
                    error_decorator(error_handler, "grid size wasn't defined", token);
                }
                else
                {
                    std::stringbuf remaining;
                    iss >> &remaining;
                    const std::string output_grid_str = extract_text_in_quotes_or_ltrim(remaining.str());
                    if (output_grid_str.size() != width * height)
                    {
                        error_decorator(error_handler, "goal size does not match the grid size", token);
                    }
                    else
                    {
                        grid.m_goal = std::make_optional<OutputGrid>(build_output_grid(output_grid_str, grid.m_name));
                    }
                }
            }
            else if (is_metadata_token(token))
            {
                std::stringbuf remaining;
                iss >> &remaining;
                grid.m_metadata.insert_or_assign(token, extract_text_in_quotes_or_ltrim(remaining.str()));
            }
            else if (is_ignored_token(token))
            {
                // Then ignore
            }
            else
            {
                error_decorator(error_handler, "Invalid token", token);
            }
        }
    }

private:
    // A blank line is described by either an empty line or a line with only a zero
    bool parse_constraint_line(std::istringstream& iss, InputGrid::Constraint& constraint)
    {
        assert(constraint.empty());
        unsigned int n;
        unsigned char c;
        while (iss >> n)
        {
            if (n == 0)
            {
                // blank line
                return constraint.empty();
            }
            else
            {
                constraint.push_back(n);
            }
            while (iss >> c)
            {
                if (c == ',')
                    break;
            }
        }
        return true;
    }

    static bool is_ignored_token(const std::string& token)
    {
        // Valid tokens for this file format, but ignored by the parser
        static const std::vector<std::string> ignored_tokens = { "color" };
        return std::find(ignored_tokens.cbegin(), ignored_tokens.cend(), token) != ignored_tokens.cend();
    }

    static bool is_metadata_token(const std::string& token)
    {
        // Valid tokens for this file format, but ignored by the parser
        static const std::vector<std::string> metadata_tokens = { "catalogue", "by", "license", "copyright" };
        return std::find(metadata_tokens.cbegin(), metadata_tokens.cend(), token) != metadata_tokens.cend();
    }

    static std::string extract_text_in_quotes_or_ltrim(const std::string& str)
    {
        const auto pos0 = str.find_first_of('"');
        const auto pos1 = str.find_last_of('"');
        if (pos0 != std::string::npos && pos1 != std::string::npos)
        {
            // Extract text in quotes
            return (pos0 + 1 < pos1) ? str.substr(pos0 + 1, pos1 - pos0 - 1) : "";
        }
        else
        {
            // Trim left spaces
            return std::string(std::find_if(str.cbegin(), str.cend(), [](unsigned char c) { return !std::isspace(c); }), str.cend());
        }
    }

    OutputGrid build_output_grid(std::string_view tiles, std::string_view name)
    {
        assert(tiles.size() == width * height);
        OutputGrid result(width, height, Tile::UNKNOWN, std::string{name});
        auto it = tiles.begin();
        // Row-major
        for (Line::Index y = 0; y < static_cast<Line::Index>(height); y++)
            for (Line::Index x = 0; x < static_cast<Line::Index>(width); x++)
            {
                assert(it != tiles.end());
                result.set_tile(x, y, *it++ == '0' ? Tile::EMPTY : Tile::FILLED);
            }
        assert(it == tiles.end());
        return result;
    }

    std::string_view parsing_state_str()
    {
        switch (parsing_state)
        {
        case ParsingState::Default:
            return "Default";
            break;
        case ParsingState::Rows:
            return "Rows";
            break;
        case ParsingState::Columns:
            return "Columns";
            break;
        default:
            assert(0);
        }
        return "UNKNOWN";
    }

    void error_decorator(const ParserErrorHandler& error_handler, std::string_view msg, std::string_view token = "")
    {
        std::ostringstream oss;
        oss << msg << " (parsing_state = " << parsing_state_str();
        if (!token.empty())
            oss << "; token = " << token;
        oss << ")";
        error_handler(oss.str());
    }
private:
    ParsingState parsing_state;
    std::size_t width;
    std::size_t height;
};


/******************************************************************************
 * Generic file parser
 ******************************************************************************/
template <typename F>
std::vector<IOGrid> parse_input_file_generic(std::string_view filepath, const ErrorHandler& error_handler) noexcept
{
    std::vector<IOGrid> result;

    try
    {
        std::vector<GridComponents> grids;
        std::ifstream inputstream(filepath.data());
        if (inputstream.is_open())
        {
            // Start line by line parsing
            FileParser<F> parser;
            auto line_stream = stdutils::io::SkipLineStream(inputstream).skip_blank_lines();
            std::string line;
            std::size_t line_nb{0u};
            while (line_stream.getline(line, line_nb))
            {
                parser.parse_line(line, grids, [&line_nb, &line, &error_handler](std::string_view msg)
                    {
                        std::ostringstream oss;
                        oss << "[" << msg << "] on line " << line_nb << ": " << line;
                        error_handler(ErrorCode::PARSING_ERROR, oss.str());
                    });
            }
            std::for_each(grids.begin(), grids.end(), [&result](GridComponents& grid_comps) {
                auto& new_grid = result.emplace_back(
                    InputGrid(std::move(grid_comps.m_rows), std::move(grid_comps.m_cols), grid_comps.m_name),
                    std::move(grid_comps.m_goal));
                for (const auto& [key, data] : grid_comps.m_metadata)
                {
                    new_grid.m_input_grid.set_metadata(key, data);
                }
            });
        }
        else
        {
            std::ostringstream oss;
            oss << "Cannot open file " << filepath;
            error_handler(ErrorCode::FILE_ERROR, oss.str());
        }
    }
    catch (std::exception& e)
    {
        std::ostringstream oss;
        oss << "Unhandled exception during file parsing: " << e.what();
        error_handler(ErrorCode::EXCEPTION, oss.str());
    }

    return result;
}

/******************************************************************************
 * Writers
 ******************************************************************************/
void write_constraints_native_format(std::ostream& out, const std::vector<InputGrid::Constraint>& constraints)
{
    for (const auto& constraint: constraints)
    {
        out << "[ ";
        std::copy(constraint.cbegin(), constraint.cend(), std::ostream_iterator<InputGrid::Constraint::value_type>(out, " "));
        out << ']' << std::endl;
    }
}

void write_constraints_nin_format(std::ostream& out, const std::vector<InputGrid::Constraint>& constraints)
{
    for (const auto& constraint : constraints)
    {
        if (constraint.empty())
        {
            out << 0 << std::endl;
        }
        else
        {
            std::copy(constraint.cbegin(), std::prev(constraint.cend()), std::ostream_iterator<InputGrid::Constraint::value_type>(out, " "));
            out << constraint.back() << std::endl;
        }
    }
}

void write_constraints_non_format(std::ostream& out, const std::vector<InputGrid::Constraint>& constraints)
{
    for (const auto& constraint : constraints)
    {
        if (constraint.empty())
        {
            out << 0 << std::endl;
        }
        else
        {
            std::copy(constraint.cbegin(), std::prev(constraint.cend()), std::ostream_iterator<InputGrid::Constraint::value_type>(out, ","));
            out << constraint.back() << std::endl;
        }
    }
}

void write_metadata_non_format(std::ostream& out, const std::map<std::string, std::string>& meta, const std::string& key, bool quoted = true)
{
    const std::string quote = quoted ? "\"" : "";
    if (meta.count(key) && !meta.at(key).empty())
    {
        out << key << " " << quote << meta.at(key) << quote << std::endl;
    }
    else if (!quote.empty())
    {
        out << key << " " << quote << quote << std::endl;
    }
    else
    {
        out << key << std::endl;
    }
}

void write_goal_non_format(std::ostream& out, const OutputGrid& goal)
{
    assert(goal.is_completed());
    const char quote = '"';
    out << "goal " << quote;
    // Tiles from top left of the puzzle along the first row, then the second row, etc.
    // 0 is a blank; anything else is a filled tile.
    for (unsigned int y = 0; y < goal.height(); y++)
        for (unsigned int x = 0; x < goal.width(); x++)
        {
            const Tile& tile = goal.get_tile(x, y);
            assert(tile == Tile::EMPTY || tile == Tile::FILLED);
            out << (tile == Tile::EMPTY ? '0' : '1');
        }
    out << quote << std::endl;
}

} // namespace


std::vector<IOGrid> parse_input_file_native(std::string_view filepath, const ErrorHandler& error_handler) noexcept
{
    return parse_input_file_generic<FileFormat::Native>(filepath, error_handler);
}

std::vector<IOGrid> parse_input_file_nin_format(std::string_view filepath, const ErrorHandler& error_handler) noexcept
{
    return parse_input_file_generic<FileFormat::Nin>(filepath, error_handler);
}

std::vector<IOGrid> parse_input_file_non_format(std::string_view filepath, const ErrorHandler& error_handler) noexcept
{
    return parse_input_file_generic<FileFormat::Non>(filepath, error_handler);
}


void write_input_grid_native(std::ostream& out, const IOGrid& grid)
{
    for (const auto& [key, data] : grid.m_input_grid.metadata())
        if (!data.empty())
            out << "# " << stdutils::string::capitalize(key) << ": " << data << std::endl;
    out << "# Size: " << str_input_grid_size(grid.m_input_grid) << std::endl;
    out << "GRID " << grid.m_input_grid.name() << std::endl;
    out << "ROWS" << std::endl;
    write_constraints_native_format(out, grid.m_input_grid.rows());
    out << "COLUMNS" << std::endl;
    write_constraints_native_format(out, grid.m_input_grid.cols());
}

void write_input_grid_nin_format(std::ostream& out, const IOGrid& grid)
{
    out << grid.m_input_grid.width() << " " << grid.m_input_grid.height() << std::endl;
    write_constraints_nin_format(out, grid.m_input_grid.rows());
    write_constraints_nin_format(out, grid.m_input_grid.cols());
}

void write_input_grid_non_format(std::ostream& out, const IOGrid& grid)
{
    constexpr bool NON_QUOTED = false;
    write_metadata_non_format(out, grid.m_input_grid.metadata(), "catalogue");
    out << "title \"" << grid.m_input_grid.name() << '\"' << std::endl;
    write_metadata_non_format(out, grid.m_input_grid.metadata(), "by");
    write_metadata_non_format(out, grid.m_input_grid.metadata(), "copyright");
    write_metadata_non_format(out, grid.m_input_grid.metadata(), "license", NON_QUOTED);

    out << "width " << grid.m_input_grid.width() << std::endl;
    out << "height " << grid.m_input_grid.height() << std::endl;

    out << std::endl;
    out << "rows" << std::endl;
    write_constraints_non_format(out, grid.m_input_grid.rows());

    out << std::endl;
    out << "columns" << std::endl;
    write_constraints_non_format(out, grid.m_input_grid.cols());

    if (grid.m_goal.has_value())
    {
        out << std::endl;
        write_goal_non_format(out, grid.m_goal.value());
    }
}

} // namespace io
} // namespace picross

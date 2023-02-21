/*******************************************************************************
 * PICROSS SOLVER
 *
 *   This file implements the file IO of the Picross solver
 *
 * Copyright (c) 2010-2022 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross_io.h>

#include "macros.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>


namespace picross
{
namespace io
{
namespace
{

constexpr unsigned int INPUT_BUFFER_SZ = 2048u;
constexpr ExitCode NO_EXIT = 0;
constexpr ExitCode EXIT_ON_INVALID_PARSER = 10;

struct FileFormat
{
    struct Native {};
    struct Nin {};
    struct Non {};
};

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

    void parse_line(const std::string& line_to_parse, std::vector<picross::InputGrid>& grids, const ErrorHandler& error_handler)
    {
        std::istringstream iss(line_to_parse);
        std::string token;

        // Copy the first word in 'token'
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
            // Comment line are ignored
        }
        else if (token.empty())
        {
            // Empty lines are ignored
        }
        else
        {
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

    void error_decorator(const ErrorHandler& error_handler, const std::string_view& msg)
    {
        std::ostringstream oss;
        oss << msg << " (parsing_state = " << parsing_state_str() << ")";
        error_handler(oss.str(), NO_EXIT);
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

    void parse_line(const std::string& line_to_parse, std::vector<picross::InputGrid>& grids, const ErrorHandler& error_handler)
    {
        UNUSED(error_handler);

        // Ignore whiteline
        if (line_to_parse.empty() || std::all_of(line_to_parse.cbegin(), line_to_parse.cend(), [](char c) { return c == '\t' || c == ' ';  } ))
            return;

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
    const char* parsing_state_str()
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

    void error_decorator(const ErrorHandler& error_handler, const std::string_view& msg)
    {
        std::ostringstream oss;
        oss << msg << " (parsing_state = " << parsing_state_str() << ")";
        error_handler(oss.str(), NO_EXIT);
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

    void parse_line(const std::string& line_to_parse, std::vector<picross::InputGrid>& grids, const ErrorHandler& error_handler)
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
                grid.m_rows.emplace_back();
                parse_constraint_line(iss, grid.m_rows.back());
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
                grid.m_cols.emplace_back();
                parse_constraint_line(iss, grid.m_cols.back());
            }
        }
        if (parsing_state == ParsingState::Default)
        {
            // Copy the first word in 'token'
            iss >> token;

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
                    error_handler("rows are already defined", NO_EXIT);
                }
                else if (height == 0)
                {
                    error_handler("height wasn't defined", NO_EXIT);
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
                    error_handler("columns are already defined", NO_EXIT);
                }
                else if (width == 0)
                {
                    error_handler("width wasn't defined", NO_EXIT);
                }
                else
                {
                    parsing_state = ParsingState::Columns;
                }
            }
            else if (token.empty())
            {
                // An empty line outside of the rows or columns sections can just be ignored
            }
            else if (is_metadata_token(token))
            {
                std::stringbuf remaining;
                iss >> &remaining;
                grid.m_metadata.insert({token, extract_text_in_quotes_or_ltrim(remaining.str())});
            }
            else if (is_ignored_token(token))
            {
                // Then ignore
            }
            else
            {
                error_handler("Invalid token " + token, NO_EXIT);
            }
        }
    }

private:
    void parse_constraint_line(std::istringstream& iss, InputGrid::Constraint& constraint)
    {
        unsigned int n;
        unsigned char c;
        while (iss >> n)
        {
            if (n != 0)
            {
                constraint.push_back(n);
            }
            while (iss >> c)
            {
                if (c == ',')
                    break;
            }
        }
    }

    static bool is_ignored_token(const std::string& token)
    {
        // Valid tokens for this file format, but ignored by the parser
        static std::vector<std::string> ignored_tokens = { "color", "goal" };
        return std::find(ignored_tokens.cbegin(), ignored_tokens.cend(), token) != ignored_tokens.cend();
    }

    static bool is_metadata_token(const std::string& token)
    {
        // Valid tokens for this file format, but ignored by the parser
        static std::vector<std::string> metadata_tokens = { "catalogue", "by", "license", "copyright" };
        return std::find(metadata_tokens.cbegin(), metadata_tokens.cend(), token) != metadata_tokens.cend();
    }

    static std::string extract_text_in_quotes_or_ltrim(const std::string& str)
    {
        const auto pos0 = str.find_first_of('"');
        const auto pos1 = str.find_last_of('"');
        if (pos0 != std::string::npos && pos1 != std::string::npos)
        {
            // Extract text in quotes
            assert(pos1 > pos0);
            return str.substr(pos0 + 1, pos1 - pos0 - 1);
        }
        else
        {
            // Trim left spaces
            return std::string(std::find_if(str.cbegin(), str.cend(), [](unsigned char c) { return !std::isspace(c); }), str.cend());
        }
    }
private:
    ParsingState parsing_state;
    std::size_t width;
    std::size_t height;
};


template <typename F>
std::vector<InputGrid> parse_input_file_generic(std::string_view filepath, const ErrorHandler& error_handler) noexcept
{
    std::vector<InputGrid> result;

    try
    {
        std::ifstream inputstream(filepath.data());
        if (inputstream.is_open())
        {
            FileParser<F> parser;
            // Line buffer
            char line[INPUT_BUFFER_SZ];
            unsigned int line_nb = 0;
            // Start parsing
            while (inputstream.good())
            {
                line_nb++;
                inputstream.getline(line, INPUT_BUFFER_SZ - 1);
                parser.parse_line(std::string(line), result, [line_nb, &line, &error_handler](std::string_view msg, ExitCode code)
                    {
                        std::ostringstream oss;
                        oss << "Parsing error [" << msg << "] on line " << line_nb << ": " << line;
                        error_handler(oss.str(), code);
                    });
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "Cannot open file " << filepath;
            error_handler(oss.str(), 1);
        }
    }
    catch (std::exception& e)
    {
        std::ostringstream oss;
        oss << "Unhandled exception during file parsing: " << e.what();
        error_handler(oss.str(), 2);
    }

    return result;
}

void write_constraints_native_format(std::ostream& ostream, const std::vector<InputGrid::Constraint>& constraints)
{
    for (const auto& constraint: constraints)
    {
        ostream << "[ ";
        std::copy(constraint.cbegin(), constraint.cend(), std::ostream_iterator<InputGrid::Constraint::value_type>(ostream, " "));
        ostream << ']' << std::endl;
    }
}

void write_constraints_nin_format(std::ostream& ostream, const std::vector<InputGrid::Constraint>& constraints)
{
    for (const auto& constraint : constraints)
    {
        if (constraint.empty())
        {
            ostream << 0 << std::endl;
        }
        else
        {
            std::copy(constraint.cbegin(), std::prev(constraint.cend()), std::ostream_iterator<InputGrid::Constraint::value_type>(ostream, " "));
            ostream << constraint.back() << std::endl;
        }
    }
}

void write_constraints_non_format(std::ostream& ostream, const std::vector<InputGrid::Constraint>& constraints)
{
    for (const auto& constraint : constraints)
    {
        if (constraint.empty())
        {
            ostream << 0 << std::endl;
        }
        else
        {
            std::copy(constraint.cbegin(), std::prev(constraint.cend()), std::ostream_iterator<InputGrid::Constraint::value_type>(ostream, ","));
            ostream << constraint.back() << std::endl;
        }
    }
}

void write_metadata_non_format(std::ostream& ostream, const std::map<std::string, std::string>& meta, const std::string& key, bool quoted = true)
{
    const std::string quote = quoted ? "\"" : "";
    if (meta.count(key) && !meta.at(key).empty())
    {
        ostream << key << ' ' << quote << meta.at(key) << quote << std::endl;
    }
    else if (!quote.empty())
    {
        ostream << key << ' ' << quote << quote << std::endl;
    }
    else
    {
        ostream << key << std::endl;
    }
}

void write_goal_non_format(std::ostream& ostream, const OutputGrid& goal)
{
    assert(goal.is_completed());
    const char quote = '"';
    ostream << "goal " << quote;
    // Tiles from top left of the puzzle along the first row, then the second row, etc.
    // 0 is a blank; anything else is a filled tile.
    for (unsigned int y = 0; y < goal.height(); y++)
        for (unsigned int x = 0; x < goal.width(); x++)
        {
            const Tile& tile = goal.get_tile(x, y);
            assert(tile == Tile::EMPTY || tile == Tile::FILLED);
            ostream << (tile == Tile::EMPTY ? '0' : '1');
        }
    ostream << quote << std::endl;
}

} // Anonymous namespace

std::vector<InputGrid> parse_input_file_native(std::string_view filepath, const ErrorHandler& error_handler) noexcept
{
    return parse_input_file_generic<FileFormat::Native>(filepath, error_handler);
}

std::vector<InputGrid> parse_input_file_nin_format(std::string_view filepath, const ErrorHandler& error_handler) noexcept
{
    return parse_input_file_generic<FileFormat::Nin>(filepath, error_handler);
}

std::vector<InputGrid> parse_input_file_non_format(std::string_view filepath, const ErrorHandler& error_handler) noexcept
{
    return parse_input_file_generic<FileFormat::Non>(filepath, error_handler);
}

void write_input_grid_native(std::ostream& ostream, const InputGrid& input_grid)
{
    for (const auto& kvp : input_grid.m_metadata)
        if (!kvp.second.empty())
            ostream << "# " << kvp.first << ": " << kvp.second << std::endl;
    ostream << "GRID " << input_grid.name() << std::endl;
    ostream << "ROWS" << std::endl;
    write_constraints_native_format(ostream, input_grid.m_rows);
    ostream << "COLUMNS" << std::endl;
    write_constraints_native_format(ostream, input_grid.m_cols);
}

void write_input_grid_nin_format(std::ostream& ostream, const InputGrid& input_grid)
{
    ostream << input_grid.width() << " " << input_grid.height();
    write_constraints_nin_format(ostream, input_grid.m_rows);
    write_constraints_nin_format(ostream, input_grid.m_cols);
}

void write_input_grid_non_format(std::ostream& ostream, const InputGrid& input_grid, std::optional<OutputGrid> goal)
{
    constexpr bool NON_QUOTED = false;
    write_metadata_non_format(ostream, input_grid.m_metadata, "catalogue");
    ostream << "title \"" << input_grid.m_name << '\"' << std::endl;
    write_metadata_non_format(ostream, input_grid.m_metadata, "by");
    write_metadata_non_format(ostream, input_grid.m_metadata, "copyright");
    write_metadata_non_format(ostream, input_grid.m_metadata, "license", NON_QUOTED);

    ostream << "width " << input_grid.m_cols.size() << std::endl;
    ostream << "height " << input_grid.m_rows.size() << std::endl;

    ostream << std::endl;
    ostream << "rows" << std::endl;
    write_constraints_non_format(ostream, input_grid.m_rows);

    ostream << std::endl;
    ostream << "columns" << std::endl;
    write_constraints_non_format(ostream, input_grid.m_cols);

    if (goal.has_value())
    {
        ostream << std::endl;
        write_goal_non_format(ostream, goal.value());
    }
}

} // namespace io
} // namespace picross

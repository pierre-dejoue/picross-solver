/*******************************************************************************
 * PICROSS SOLVER
 *
 *   This file implements the file IO of the Picross solver
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross_io.h>


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
    struct Non {};
};

template <typename F>
class FileParser
{
public:
    bool parse_line(const std::string& line_to_parse, std::vector<picross::InputGrid>& grids, const ErrorHandler& error_handler)
    {
        error_handler("Invalid file parser", EXIT_ON_INVALID_PARSER);
    }
};

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

    bool parse_line(const std::string& line_to_parse, std::vector<picross::InputGrid>& grids, const ErrorHandler& error_handler)
    {
        bool valid_line = false;
        std::istringstream iss(line_to_parse);
        std::string token;

        /* Copy the first word in 'token' */
        iss >> token;

        if (token == "GRID")
        {
            parsing_state = ParsingState::GRID_START;
            grids.emplace_back();
            if (line_to_parse.size() > 5u)
            {
                grids.back().name = line_to_parse.substr(5u);
            }
            valid_line = true;
        }
        else if (token == "ROWS")
        {
            if (parsing_state != ParsingState::FILE_START)
            {
                parsing_state = ParsingState::ROW_SECTION;
                valid_line = true;
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
                valid_line = true;
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
                grids.back().rows.push_back(std::move(new_row));
                valid_line = true;
            }
            else if (parsing_state == ParsingState::COLUMN_SECTION)
            {
                picross::InputGrid::Constraint new_col;
                unsigned int n;
                while (iss >> n) { new_col.push_back(n); }
                grids.back().cols.push_back(std::move(new_col));
                valid_line = true;
            }
            else
            {
                error_decorator(error_handler, "Unexpected token " + token);
            }
        }
        else if (token == "#")
        {
            // Comment line
            valid_line = true;
        }
        else if (token.empty())
        {
            // Empty line is ignored
            valid_line = true;
        }
        else
        {
            error_decorator(error_handler, "Invalid token " + token);
        }

        return valid_line;
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

    void error_decorator(const ErrorHandler& error_handler, const std::string& msg)
    {
        std::ostringstream oss;
        oss << msg << " (parsing_state = " << parsing_state_str() << ")";
        error_handler(oss.str(), NO_EXIT);
    }
private:
    ParsingState parsing_state;
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

    bool parse_line(const std::string& line_to_parse, std::vector<picross::InputGrid>& grids, const ErrorHandler& error_handler)
    {
        bool valid_line = false;
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
            if (grid.rows.size() == height)
            {
                parsing_state = ParsingState::Default;
            }
            else
            {
                grid.rows.emplace_back();
                valid_line = parse_constraint_line(iss, grid.rows.back());
            }

        }
        if (parsing_state == ParsingState::Columns)
        {
            if (grid.cols.size() == width)
            {
                parsing_state = ParsingState::Default;
            }
            else
            {
                grid.cols.emplace_back();
                valid_line = parse_constraint_line(iss, grid.cols.back());
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
                grid.name = extract_text_in_quotes_or_ltrim(remaining.str());
                valid_line = true;
            }
            else if (token == "width")
            {
                iss >> width;
                valid_line = true;
            }
            else if (token == "height")
            {
                iss >> height;
                valid_line = true;
            }
            else if (token == "rows")
            {
                if (!grid.rows.empty())
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
                    valid_line = true;
                }
            }
            else if (token == "columns")
            {
                if (!grid.cols.empty())
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
                    valid_line = true;
                }
            }
            else if (token.empty())
            {
                // An empty line outside of the rows or columns sections can just be ignored
                valid_line = true;
            }
            else if (is_metadata_token(token))
            {
                std::stringbuf remaining;
                iss >> &remaining;
                grid.metadata.insert_or_assign(token, extract_text_in_quotes_or_ltrim(remaining.str()));
                valid_line = true;
            }
            else if (is_ignored_token(token))
            {
                // Then ignore
                valid_line = true;
            }
            else
            {
                error_handler("Invalid token " + token, NO_EXIT);
            }
        }

        return valid_line;
    }

private:
    bool parse_constraint_line(std::istringstream& iss, InputGrid::Constraint& constraint)
    {
        bool valid_line = true;
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
        return valid_line;
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
std::vector<InputGrid> parse_input_file_generic(const std::string& filepath, const ErrorHandler& error_handler) noexcept
{
    std::vector<InputGrid> result;

    try
    {
        std::ifstream inputstream(filepath);
        if (inputstream.is_open())
        {
            FileParser<F> parser;
            /* Line buffer */
            char line[INPUT_BUFFER_SZ];
            unsigned int line_nb = 0;
            /* Start parsing */
            while (inputstream.good())
            {
                line_nb++;
                inputstream.getline(line, INPUT_BUFFER_SZ - 1);
                parser.parse_line(std::string(line), result, [line_nb, &line, &error_handler](const std::string& msg, ExitCode code)
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

inline void write_metadata_non_format(std::ostream& ostream, const std::map<std::string, std::string>& meta, const std::string& key, const std::string& quote = "\"")
{
    if (meta.count(key)) { ostream << key << ' ' << quote << meta.at(key) << quote << std::endl; }
}

} // anonymous namespace

std::vector<InputGrid> parse_input_file(const std::string& filepath, const ErrorHandler& error_handler) noexcept
{
    return parse_input_file_generic<FileFormat::Native>(filepath, error_handler);
}

std::vector<InputGrid> parse_input_file_non_format(const std::string& filepath, const ErrorHandler& error_handler) noexcept
{
    return parse_input_file_generic<FileFormat::Non>(filepath, error_handler);
}

void write_input_grid(std::ostream& ostream, const InputGrid& input_grid)
{
    for (const auto& kvp : input_grid.metadata)
    {
        ostream << "# " << kvp.first << ' ' << kvp.second << std::endl;
    }
    ostream << "GRID " << input_grid.name << std::endl;
    ostream << "ROWS" << std::endl;
    write_constraints_native_format(ostream, input_grid.rows);
    ostream << "COLUMNS" << std::endl;
    write_constraints_native_format(ostream, input_grid.cols);
}


void write_input_grid_non_format(std::ostream& ostream, const InputGrid& input_grid)
{
    write_metadata_non_format(ostream, input_grid.metadata, "catalog");
    ostream << "title \"" << input_grid.name << '\"' << std::endl;
    write_metadata_non_format(ostream, input_grid.metadata, "by");
    write_metadata_non_format(ostream, input_grid.metadata, "copyright");
    write_metadata_non_format(ostream, input_grid.metadata, "license", "");

    ostream << "width " << input_grid.cols.size() << std::endl;
    ostream << "height " << input_grid.rows.size() << std::endl;

    ostream << std::endl;
    ostream << "rows" << std::endl;
    write_constraints_non_format(ostream, input_grid.rows);

    ostream << std::endl;
    ostream << "cols" << std::endl;
    write_constraints_non_format(ostream, input_grid.cols);
}

} // namespace io
} // namespace picross
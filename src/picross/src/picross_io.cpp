/*******************************************************************************
 * PICROSS SOLVER
 *
 *   This file implements the file IO of the Picross solver
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross_io.h>


#include <cassert>
#include <fstream>
#include <sstream>


namespace picross
{
namespace
{
    constexpr unsigned int INPUT_BUFFER_SZ = 2048u;
    constexpr ExitCode NO_EXIT = 0;

    class FileParser
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
                grids.push_back(picross::InputGrid());
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

} // anonymous namespace


std::vector<InputGrid> parse_input_file(const std::string& filepath, const ErrorHandler& error_handler) noexcept
{
    std::vector<InputGrid> result;

    try
    {
        std::ifstream inputstream(filepath);
        if (inputstream.is_open())
        {
            FileParser parser;
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
                        oss << "Parsing error \"" << msg << "\" on line " << line_nb << ": " << line;
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
        oss << "Unhandled exception: " << e.what();
        error_handler(oss.str(), 2);
    }

    return result;
}

} // namespace picross
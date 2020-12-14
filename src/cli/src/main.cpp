/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Implementation of a CLI for the solver of Picross puzzles (nonograms)
 *   Parses the input text file with the information for one or several grids
 *   and solve them. If several solutions exist, an exhaustive search is done
 *   and all of them are displayed.
 *
 * Copyright (c) 2010-2020 Pierre DEJOUE
 ******************************************************************************/
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>


#include <picross/picross.h>


enum ParsingState
{
    FILE_START,
    GRID_START,
    ROW_SECTION,
    COLUMN_SECTION
};


ParsingState parsing_state = FILE_START;
const int INPUT_BUFFER_SZ = 2048;


bool parse_picross_input_file(const std::string& line_to_parse, std::vector<picross::InputGrid>& grids);


/*******************************************************************************
 * MAIN()
 ******************************************************************************/
int main(int argc, char *argv[])
{
    /***************************************************************************
     * I - Process command line
     **************************************************************************/
    std::ifstream inputstream;
    char * filename;
    if(argc == 2)
    {
        filename = argv[1];  // input filename
        inputstream.open(filename);
        if(!inputstream.is_open())
        {
            std::cerr << "Cannot open file " << filename << std::endl;
            exit(1);
        }
    }
    else
    {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "    picross_parser input_filename" << std::endl;
        exit(1);
    }

    /***************************************************************************
     * II - Parse input file
     **************************************************************************/

    /* Line buffer */
    char line[INPUT_BUFFER_SZ];
    /* Container for grids input data */
    std::vector<picross::InputGrid> grids_to_solve;
    int line_nb = 0;
    bool is_line_ok;
    /* Start parsing */
    while(inputstream.good())
    {
        line_nb++;
        inputstream.getline(line, INPUT_BUFFER_SZ - 1);
        if((is_line_ok = parse_picross_input_file(std::string(line), grids_to_solve)) == false)
        {
            std::cerr << "Parsing error on line " << line_nb << " (parsing_state = " << parsing_state << "): " << line << std::endl;
        }
    }
    /* Close file */
    inputstream.close();

    /***************************************************************************
     * III - Solve Picross puzzles.
     **************************************************************************/
    try
    {
        const auto solver = picross::getRefSolver();

        unsigned int count_grids = 0u;
        for(auto& grid_input = grids_to_solve.begin(); grid_input != grids_to_solve.end(); ++grid_input)
        {
            std::cout << "GRID " << ++count_grids << ": " << grid_input->name << std::endl;

            /* Sanity check of the input data */
            bool check;
            std::string check_msg;
            std::tie(check, check_msg) = check_grid_input(*grid_input);
            if(check)
            {
                /* Solve the grid */
                picross::GridStats stats;
                std::vector<std::unique_ptr<picross::SolvedGrid>> solutions = solver->solve(*grid_input, &stats);

                /* Display solutions */
                if(solutions.empty())
                {
                    std::cout << " > Could not solve that grid :-(" << std::endl << std::endl;
                }
                else
                {
                    std::cout << " > Found " << solutions.size() << " solution(s):" << std::endl << std::endl;
                    for(auto& solution = solutions.cbegin(); solution != solutions.cend(); ++solution) { (*solution)->print(std::cout); }
                }

                /* Display stats */
                print_grid_stats(&stats, std::cout);
                std::cout << std::endl;
            }
            else
            {
                std::cout << " > Invalid grid. Error message: " << check_msg << std::endl;
            }
            std::cout << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "UNEXPECTED ERROR." << std::endl;
    }

    /***************************************************************************
     * IV - Exit
     **************************************************************************/
    return 0;
}


/*******************************************************************************
 * parse_picross_input_file: parse one line from the input file, based on parsing_state
 *
 *    file format:
 *      GRID <title>        <--- beginning of a new grid
 *      ROWS                <--- marker to start listing the constraints on the rows
 *      [ 1 2 3 ...         <--- constraint on one line (here a row)
 *      ...
 *      COLUMNS             <--- marker for columns
 *      ...
 ******************************************************************************/
bool parse_picross_input_file(const std::string& line_to_parse, std::vector<picross::InputGrid>& grids)
{
    bool valid_line = false;
    std::istringstream iss(line_to_parse);
    std::string token;

    /* Copy the first word in 'token' */
    iss >> token;

    if(token ==  "GRID")
    {
        parsing_state = GRID_START;
        valid_line = true;
        grids.push_back(picross::InputGrid());

        grids.back().name = line_to_parse.substr(5);
    }
    else if(token == "ROWS")
    {
        if(parsing_state != FILE_START)
        {
            parsing_state = ROW_SECTION;
            valid_line = true;
        }
    }
    else if(token == "COLUMNS")
    {
        if(parsing_state != FILE_START)
        {
            parsing_state = COLUMN_SECTION;
            valid_line = true;
        }
    }
    else if(token == "[")
    {
        if(parsing_state == ROW_SECTION)
        {
            picross::InputConstraint new_row;
            unsigned int n;
            while(iss >> n) { new_row.push_back(n); }
            grids.back().rows.push_back(std::move(new_row));
            valid_line = true;
        }
        else if(parsing_state == COLUMN_SECTION)
        {
            picross::InputConstraint new_column;
            unsigned int n;
            while(iss >> n) { new_column.push_back(n); }
            grids.back().columns.push_back(std::move(new_column));
            valid_line = true;
        }
    }
    else if(token == "")
    {
        // Empty line
        valid_line = true;
    }

    return valid_line;
}

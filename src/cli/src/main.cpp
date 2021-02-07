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
#include <iostream>
#include <tuple>
#include <vector>

#include <picross/picross.h>
#include <picross/picross_io.h>


/*******************************************************************************
 * MAIN()
 ******************************************************************************/
int main(int argc, char *argv[])
{
    /***************************************************************************
     * I - Process command line
     **************************************************************************/
    std::string filename;
    if(argc == 2)
    {
        filename = argv[1];     // input filename;
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

    std::vector<picross::InputGrid> grids_to_solve = picross::parse_input_file(filename, [](const std::string& msg, picross::ExitCode code)
    {
        std::cerr << msg << std::endl;
        if (code != 0)
        {
            exit(code);
        }
    });

    /***************************************************************************
     * III - Solve Picross puzzles
     **************************************************************************/
    try
    {
        const auto solver = picross::get_ref_solver();

        unsigned int count_grids = 0u;
        for(auto& grid_input = grids_to_solve.begin(); grid_input != grids_to_solve.end(); ++grid_input)
        {
            std::cout << "GRID " << ++count_grids << ": " << grid_input->name << std::endl;

            /* Sanity check of the input data */
            bool check;
            std::string check_msg;
            std::tie(check, check_msg) = picross::check_grid_input(*grid_input);
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
                picross::print_grid_stats(&stats, std::cout);
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
        std::cerr << "ERROR: " << e.what() << std::endl;
    }

    /***************************************************************************
     * IV - Exit
     **************************************************************************/
    return 0;
}


/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Implementation of a CLI for the solver of Picross puzzles (nonograms)
 *   Parses the input text file with the information for one or several grids
 *   and solve them. If several solutions exist, an exhaustive search is done
 *   and all of them are displayed.
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include <cassert>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>

#include <argagg/argagg.hpp>

#include <picross/picross.h>
#include <picross/picross_io.h>
#include <utils/console_observer.h>
#include <utils/duration_meas.h>
#include <utils/strings.h>


/*******************************************************************************
 * MAIN()
 ******************************************************************************/
int main(int argc, char *argv[])
{
    /***************************************************************************
     * I - Process command line
     **************************************************************************/
    argagg::parser argparser
    {{
      {
        "help", { "-h", "--help" },
        "Print usage note and exit", 0 },
      {
        "no-timing", { "--no-timing" },
        "Do not print out timing measurements", 0 },
      {
        "verbose", { "-v", "--verbose" },
        "Print additional debug information", 0 },
      {
        "max-nb-solutions", { "--max-nb-solutions" },
        "Limit the number of solutions returned per grid", 1 }
    }};

    std::ostringstream usage_note;
    usage_note << "Usage:" << std::endl;
    usage_note << "    picross_solver_cli [options] FILES" << std::endl;
    usage_note << argparser;

    argagg::parser_results args;
    try
    {
        args = argparser.parse(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << usage_note.str();
        std::cerr << std::endl;
        std::cerr << "Exception while parsing arguments: " << e.what() << std::endl;
        exit(1);
    }

    if (args["help"])
    {
        std::cerr << usage_note.str();
        exit(0);
    }

    const auto max_nb_solutions = args["max-nb-solutions"].as<unsigned int>(0u);

    // Positional arguments
    if (args.pos.empty())
    {
        std::cerr << usage_note.str();
        exit(1);
    }

    int return_status = 0;
    unsigned int count_grids = 0u;

    for (const std::string& filepath : args.pos)
    {
        const std::string filename = file_name(filepath);

        /***************************************************************************
         * II - Parse input file
         **************************************************************************/
        const auto err_handler = [&return_status, &filename](const std::string& msg, picross::ExitCode code)
        {
            std::cout << (code == 0 ? "WARNING" : "ERROR" ) << " [" << filename << "]: " << msg << std::endl;
            if (code != 0)
                return_status = code;
        };
        const std::vector<picross::InputGrid> grids_to_solve = str_tolower(file_extension(filepath)) == "non"
            ? picross::parse_input_file_non_format(filepath, err_handler)
            : picross::parse_input_file(filepath, err_handler);

        /***************************************************************************
         * III - Solve Picross puzzles
         **************************************************************************/
        try
        {
            const auto solver = picross::get_ref_solver();

            for (const auto& grid_input : grids_to_solve)
            {
                std::cout << "GRID " << ++count_grids << ": " << grid_input.name << std::endl;

                /* Sanity check of the input data */
                bool check;
                std::string check_msg;
                std::tie(check, check_msg) = picross::check_grid_input(grid_input);
                if (check)
                {
                    /* Set observer */
                    const auto width = grid_input.cols.size();
                    const auto height = grid_input.rows.size();
                    ConsoleObserver obs(width, height, std::cout);
                    if (args["verbose"])
                    {
                        solver->set_observer(std::reference_wrapper<ConsoleObserver>(obs));
                    }
                    else
                    {
                        // Set a dummy observer just to collect stats
                        solver->set_observer([](picross::Solver::Event, const picross::Line*, unsigned int) {});
                    }

                    /* Solve the grid */
                    picross::GridStats stats;
                    solver->set_stats(stats);
                    std::chrono::duration<float, std::milli> time_ms;
                    std::vector<picross::OutputGrid> solutions;
                    {
                        DurationMeas<float, std::milli> meas_ms(time_ms);
                        solutions = solver->solve(grid_input, max_nb_solutions);
                    }

                    /* Display solutions */
                    if (solutions.empty())
                    {
                        std::cout << " > Could not solve that grid :-(" << std::endl << std::endl;
                    }
                    else
                    {
                        std::cout << " > Found " << solutions.size() << " solution(s):" << std::endl << std::endl;
                        for (const auto& solution : solutions)
                        {
                            assert(solution.is_solved());
                            std::cout << solution << std::endl;
                        }
                    }

                    /* Display stats */
                    std::cout << stats << std::endl;

                    /* Display timings */
                    if (!args["no-timing"])
                    {
                        std::cout << "  Solver wall time: " << time_ms.count() << "ms" << std::endl;
                    }

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
            std::cout << "EXCPT [" << filename << "]: " << e.what() << std::endl;
            return_status = 5;
        }
    }

    /***************************************************************************
     * IV - Exit
     **************************************************************************/
    return return_status;
}


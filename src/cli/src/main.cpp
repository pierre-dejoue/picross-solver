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


namespace
{

    const std::string validation_mode_format = "File,Grid,Size,Valid,Difficulty,Timing (ms),Misc";

    struct ValidationModeData
    {
        ValidationModeData();

        std::string filename;
        std::string grid;
        std::string size;
        picross::ValidationCode validation_code;
        unsigned int max_search_depth;
        float timing_ms;
        std::string misc;
    };

    ValidationModeData::ValidationModeData() : filename(), grid(), size(), validation_code(-1), max_search_depth(0u), timing_ms(0u), misc()
    {
    }

    std::ostream& operator<<(std::ostream& ostream, const ValidationModeData& data)
    {
        ostream << data.filename << ',';
        ostream << data.grid << ',';
        ostream << data.size << ',';
        ostream << picross::validation_code_str(data.validation_code) << ',';
        ostream << (data.validation_code == 1 && (data.max_search_depth == 0u) ? "LINE" : "") << ',';
        ostream << data.timing_ms;
        if (!data.misc.empty())
            ostream << ",\"" << data.misc << "\"";
        return ostream;
    }

} // namespace

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
        "Limit the number of solutions returned per grid", 1 },
      {
        "validation-mode", { "--validation" },
        "Validation mode: Check for unique solution, output one line per grid", 0 }
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
    const bool validation_mode = args["validation-mode"];

    // Positional arguments
    if (args.pos.empty())
    {
        std::cerr << usage_note.str();
        exit(1);
    }

    int return_status = 0;
    unsigned int count_grids = 0u;

    if (validation_mode) { std::cout << validation_mode_format << std::endl; }

    /* Solver */
    const auto solver = picross::get_ref_solver();


    /***************************************************************************
     * II - Parse input files
     **************************************************************************/
    for (const std::string& filepath : args.pos)
    {
        ValidationModeData file_data;
        file_data.filename = file_name(filepath);

        const picross::io::ErrorHandler err_handler_classic = [&return_status, &file_data](std::string_view msg, picross::io::ExitCode code)
        {
            std::cout << (code == 0 ? "WARNING" : "ERROR" ) << " [" << file_data.filename << "]: " << msg << std::endl;
            if (code != 0)
                return_status = code;
        };
        const picross::io::ErrorHandler err_handler_validation = [&file_data](std::string_view msg, picross::io::ExitCode code)
        {
            file_data.misc = msg.empty() ? std::to_string(code) : msg;
        };

        const std::vector<picross::InputGrid> grids_to_solve = str_tolower(file_extension(filepath)) == "non"
            ? picross::io::parse_input_file_non_format(filepath, (validation_mode ? err_handler_validation : err_handler_classic))
            : picross::io::parse_input_file(filepath, (validation_mode ? err_handler_validation : err_handler_classic));

        if (validation_mode && !file_data.misc.empty()) { std::cout << file_data << std::endl; }


        /***************************************************************************
         * III - Solve Picross puzzles
         **************************************************************************/
        for (const auto& grid_input : grids_to_solve)
        {
            ValidationModeData grid_data = file_data;
            grid_data.grid = grid_input.name;
            grid_data.size = grid_size_str(grid_input);

            try
            {
                if (!validation_mode)
                {
                    std::cout << "GRID " << ++count_grids << ": " << grid_input.name << std::endl;
                    std::cout << "  Size: " << grid_data.size << std::endl;
                }

                /* Sanity check of the input data */
                bool input_ok;
                std::tie(input_ok, grid_data.misc) = picross::check_grid_input(grid_input);
                grid_data.validation_code = input_ok ? 0 : -1;

                if (input_ok)
                {
                    /* Set observer */
                    const auto width = grid_input.cols.size();
                    const auto height = grid_input.rows.size();
                    ConsoleObserver obs(width, height, std::cout);
                    if (args["verbose"])
                    {
                        solver->set_observer(std::reference_wrapper<ConsoleObserver>(obs));
                    }
                    else if (!validation_mode)
                    {
                        // Set a dummy observer just to collect stats on number of observer calls
                        solver->set_observer([](picross::Solver::Event, const picross::Line*, unsigned int) {});
                    }

                    /* Reset stats */
                    picross::GridStats stats;
                    solver->set_stats(stats);

                    std::chrono::duration<float, std::milli> time_ms;
                    if (validation_mode)
                    {
                        /* Validate the grid */
                        {
                            DurationMeas<float, std::milli> meas_ms(time_ms);
                            std::tie(grid_data.validation_code, grid_data.misc) = picross::validate_input_grid(*solver, grid_input);
                        }
                        grid_data.max_search_depth = stats.max_nested_level;
                        grid_data.timing_ms = time_ms.count();
                    }
                    else
                    {
                        /* Solve the grid */
                        picross::Solver::Status status;
                        std::vector<picross::OutputGrid> solutions;
                        {
                            DurationMeas<float, std::milli> meas_ms(time_ms);
                            std::tie(status, solutions) = solver->solve(grid_input, max_nb_solutions);
                        }

                        /* Display solutions */
                        if (status != picross::Solver::Status::OK)
                        {
                            std::cout << "  Could not solve that grid :-(" << std::endl << std::endl;
                        }
                        else
                        {
                            std::cout << "  Found " << solutions.size() << " solution(s)" << std::endl << std::endl;
                            int nb = 0;
                            for (const auto& solution : solutions)
                            {
                                assert(solution.is_solved());
                                std::cout << "  Nb " << ++nb << ":" << std::endl;
                                std::cout << solution << std::endl;
                            }
                        }

                        /* Display stats */
                        std::cout << stats << std::endl;

                        /* Display timings */
                        if (!args["no-timing"])
                        {
                            std::cout << "  Wall time: " << time_ms.count() << "ms" << std::endl;
                        }
                    }
                }
                else if (!validation_mode)
                {
                    std::cout << "  Invalid grid. Error message: " << grid_data.misc << std::endl;
                }
            }
            catch (std::exception& e)
            {
                if (validation_mode)
                {
                    grid_data.validation_code = -1;
                    grid_data.misc = "EXCPT " + std::string(e.what());
                }
                else
                {
                    std::cout << "EXCPT [" << file_data.filename << "][" << grid_input.name << "]: " << e.what() << std::endl;
                    return_status = 5;
                }
            }

            if (validation_mode)
            {
                std::cout << grid_data << std::endl;
            }
            else
            {
                std::cout << std::endl << std::endl;
            }
        }
    }


    /***************************************************************************
     * IV - Exit
     **************************************************************************/
    return return_status;
}


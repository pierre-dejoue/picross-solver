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
#include <picross/picross.h>
#include <picross/picross_io.h>
#include <utils/console_observer.h>
#include <utils/duration_meas.h>
#include <utils/strings.h>

#include <argagg/argagg.hpp>

#include <cassert>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>


namespace
{

    const std::string validation_mode_format = "File,Grid,Size,Valid,Difficulty,Timing (ms),Misc";

    struct ValidationModeData
    {
        ValidationModeData();

        std::string filename;
        std::string grid;
        std::string size;
        picross::ValidationResult validation_result;
        float timing_ms;
        std::string misc;
    };

    ValidationModeData::ValidationModeData() : filename(), grid(), size(), validation_result{ -1, 0u, "" }, timing_ms(0u), misc()
    {
    }

    std::ostream& operator<<(std::ostream& ostream, const ValidationModeData& data)
    {
        ostream << data.filename << ',';
        ostream << data.grid << ',';
        ostream << data.size << ',';
        ostream << picross::validation_code_str(data.validation_result.code) << ',';
        ostream << (data.validation_result.code == 1 && (data.validation_result.branching_depth == 0u) ? "LINE" : "") << ',';
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
    { {
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
        "Validation mode: Check for unique solution, output one line per grid", 0 },
      {
        "line-solver", { "--line-solver" },
        "Line solver: might output an incomplete solution if the grid is not line solvable", 0 }
    } };

    std::ostringstream usage_note;
    usage_note << "Picross Solver " << picross::get_version_string() << std::endl;
    usage_note << std::endl;
    usage_note << "Usage:" << std::endl;
    usage_note << "    picross_solver_cli [options] FILES" << std::endl;
    usage_note << std::endl;
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
    const auto solver = args["line-solver"] ? picross::get_line_solver() : picross::get_ref_solver();


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
                grid_data.validation_result.code = input_ok ? 0 : -1;

                if (input_ok)
                {
                    /* Set observer */
                    const auto width = grid_input.cols.size();
                    const auto height = grid_input.rows.size();
                    ConsoleObserver obs(width, height, std::cout);
                    if (!validation_mode)
                    {
                        if (args["verbose"])
                        {
                            solver->set_observer(std::reference_wrapper<ConsoleObserver>(obs));
                        }
                    }

                    std::chrono::duration<float, std::milli> time_ms;
                    if (validation_mode)
                    {
                        /* Validate the grid */
                        {
                            DurationMeas<float, std::milli> meas_ms(time_ms);
                            grid_data.validation_result = picross::validate_input_grid(*solver, grid_input);
                        }
                        grid_data.timing_ms = time_ms.count();
                    }
                    else
                    {
                        /* Reset stats */
                        picross::GridStats stats;
                        solver->set_stats(stats);

                        /* Solve the grid */
                        picross::Solver::Result solver_result;
                        {
                            DurationMeas<float, std::milli> meas_ms(time_ms);
                            solver_result = solver->solve(grid_input, max_nb_solutions);
                        }

                        /* Display solutions */
                        if (solver_result.status != picross::Solver::Status::OK)
                        {
                            std::cout << "  Could not solve that grid :-(" << std::endl;
                            if (solver_result.status == picross::Solver::Status::NOT_LINE_SOLVABLE)
                            {
                                assert(solver_result.solutions.size() == 1);
                                const auto& solution = solver_result.solutions.front();
                                assert(!solution.grid.is_solved());
                                std::cout << "  Found partial solution" << std::endl;
                                std::cout << solution.grid << std::endl;
                            }
                            std::cout << std::endl;
                        }
                        else
                        {
                            std::cout << "  Found " << solver_result.solutions.size() << " solution(s)" << std::endl;
                            std::cout << std::endl;
                            int nb = 0;
                            for (const auto& solution : solver_result.solutions)
                            {
                                assert(solution.grid.is_solved());
                                std::cout << "  Nb " << ++nb << ": (branching depth: " << solution.branching_depth << ")" << std::endl;
                                std::cout << solution.grid << std::endl;
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
                    grid_data.validation_result.code = -1;
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


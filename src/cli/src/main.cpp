/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Implementation of a CLI for the solver of Picross puzzles (nonograms)
 *   Parses the input text file with the information for one or several grids
 *   and solve them. If several solutions exist, an exhaustive search is done
 *   and all of them are displayed.
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#include <picross/picross.h>
#include <utils/compiler_info.h>
#include <utils/console_observer.h>
#include <utils/console_progress_observer.h>
#include <utils/duration_meas.h>
#include <utils/input_grid_utils.h>
#include <utils/picross_file_io.h>
#include <utils/strings.h>
#include <utils/timeout.h>

#include <argagg/argagg.hpp>

#include <cassert>
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>


namespace
{

    const std::string validation_mode_format = "File,Grid,Size,Valid,Difficulty,Timing (ms),Misc,Linear reduction,Full reduction,Depth,Search alts";

    struct ValidationModeData
    {
        ValidationModeData();

        std::string filename;
        std::string gridname;
        std::string size;
        picross::ValidationResult validation_result;
        float timing_ms;
        std::optional<picross::GridStats> grid_stats;
        std::string misc;
    };

    ValidationModeData::ValidationModeData() : filename(), gridname(), size(), validation_result{ -1, 0u, "" }, timing_ms(-1.f), misc()
    {
    }

    std::ostream& operator<<(std::ostream& ostream, const ValidationModeData& data)
    {
        ostream << data.filename << ',';
        ostream << data.gridname << ',';
        ostream << data.size << ',';
        ostream << picross::str_validation_code(data.validation_result.code) << ',';
        ostream << (data.validation_result.code == 1 && (data.validation_result.branching_depth == 0u) ? "LINE" : "") << ',';
        if (data.timing_ms >= 0.f)
            ostream << data.timing_ms;
        ostream << ",";
        if (!data.validation_result.msg.empty() || !data.misc.empty())
            ostream << "\"" << (data.misc.empty() ? data.validation_result.msg : data.misc) << "\"";
        if (data.grid_stats.has_value())
        {
            ostream << "," << (data.grid_stats->nb_single_line_linear_reduction + data.grid_stats->nb_single_line_linear_reduction_w_change);
            ostream << "," << (data.grid_stats->nb_single_line_full_reduction   + data.grid_stats->nb_single_line_full_reduction_w_change);
            ostream << "," << data.grid_stats->max_branching_depth;
            ostream << "," << (data.grid_stats->total_nb_branching_alternatives + data.grid_stats->total_nb_probing_alternatives);
        }
        return ostream;
    }

    void output_solution_grid(std::ostream& ostream, const picross::OutputGrid& grid, unsigned int indent = 0)
    {
        const std::string ind(indent, ' ');
        for (unsigned int y = 0u; y < grid.height(); y++)
            ostream << ind << grid.get_line<picross::Line::ROW>(y) << std::endl;
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
        "version", { "--version" },
        "Print version and exit", 0 },
      {
        "no-timing", { "--no-timing" },
        "Do not print out timing measurements", 0 },
      {
        "verbose", { "-v", "--verbose" },
        "Print additional debug information", 0 },
      {
        "progress", { "--progress" },
        "Print branching progress in percent", 0 },
      {
        "max-nb-solutions", { "--max-nb-solutions" },
        "Limit the number of solutions returned per grid", 1 },
      {
        "validation-mode", { "--validation" },
        "Validation mode: Check for unique solution, output one line per grid", 0 },
      {
        "line-solver", { "--line-solver" },
        "Line solver: might output an incomplete solution if the grid is not line solvable", 0 },
      {
        "timeout", { "--timeout" },
        "Timeout on grid solve, in seconds", 1 },
      {
        "from_output", { "--from-output" },
        "The input is a text file with an output grid", 0 }
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

    if (args["version"])
    {
        std::cerr << "Picross Solver " << picross::get_version_string() << std::endl;
        std::cerr << std::endl;
        print_compiler_info(std::cerr);
        exit(0);
    }

    const auto max_nb_solutions = std::max(1u, args["max-nb-solutions"].as<unsigned int>(std::numeric_limits<unsigned int>::max()));
    const bool validation_mode = args["validation-mode"];
    const std::chrono::seconds timeout_duration(args["timeout"].as<unsigned int>(0u));

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
    for (const char* filepath : args.pos)
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

        const auto format = [&args, &filepath]() -> picross::io::PicrossFileFormat {
            if (args["from_output"])
                return picross::io::PicrossFileFormat::OutputGrid;
            else
                return picross::io::picross_file_format_from_filepath(filepath);
        }();

        std::optional<picross::OutputGrid> goal;
        const auto grids_to_solve = picross::io::parse_picross_file(filepath, format, goal, (validation_mode ? err_handler_validation : err_handler_classic));;

        if (validation_mode && !file_data.misc.empty()) { std::cout << file_data << std::endl; }

        /***************************************************************************
         * III - Solve Picross puzzles
         **************************************************************************/
        for (const auto& input_grid : grids_to_solve)
        {
            ValidationModeData grid_data = file_data;
            grid_data.gridname = input_grid.name();
            grid_data.size = picross::str_input_grid_size(input_grid);

            try
            {
                if (!validation_mode)
                {
                    std::cout << "GRID " << ++count_grids << ": " << input_grid.name() << std::endl;
                    std::cout << "  Size: " << grid_data.size << std::endl;
                }

                /* Sanity check of the input data */
                const auto [input_ok, check_msg] = picross::check_input_grid(input_grid);
                grid_data.validation_result.code = input_ok ? 0 : -1;
                grid_data.misc = check_msg;

                if (input_ok)
                {
                    if (args["verbose"] && !validation_mode)
                    {
                        stream_input_grid_constraints(std::cout, input_grid);
                    }

                    /* Set observer */
                    const auto width = input_grid.width();
                    const auto height = input_grid.height();
                    ConsoleObserver obs(width, height, std::cout);
                    if (goal.has_value())
                    {
                        obs.verify_against_goal(*goal);
                    }
                    ConsoleProgressObserver progress_obs(std::cout);
                    if (!validation_mode)
                    {
                        if (args["verbose"])
                        {
                            solver->set_observer(std::reference_wrapper<ConsoleObserver>(obs));
                        }
                        else if (args["progress"])
                        {
                            solver->set_observer(std::reference_wrapper<ConsoleProgressObserver>(progress_obs));
                        }
                    }

                    /* Set timeout */
                    if (timeout_duration > std::chrono::seconds::zero())
                    {
                        Timeout<std::chrono::seconds> timeout_clock(timeout_duration);
                        solver->set_abort_function([&timeout_clock]() { return timeout_clock.has_expired(); });
                    }

                    std::chrono::duration<float, std::milli> time_ms;
                    if (validation_mode)
                    {
                        /* Stats */
                        picross::GridStats stats;
                        if (args["verbose"])
                            solver->set_stats(stats);

                        /* Validate the grid */
                        {
                            DurationMeas<float, std::milli> meas_ms(time_ms);
                            grid_data.validation_result = picross::validate_input_grid(*solver, input_grid);
                        }
                        if (!args["no-timing"])
                            grid_data.timing_ms = time_ms.count();
                        if (args["verbose"])
                            grid_data.grid_stats = stats;
                    }
                    else
                    {
                        /* Stats */
                        picross::GridStats stats;
                        solver->set_stats(stats);

                        /* Solution display */
                        unsigned int nb = 0;
                        picross::Solver::SolutionFound solution_found = [&nb, max_nb_solutions](picross::Solver::Solution&& solution)
                        {
                            if (solution.partial)
                            {
                                assert(!solution.grid.is_completed());
                                std::cout << "  Partial solution:" << std::endl;
                            }
                            else
                            {
                                assert(solution.grid.is_completed());
                                std::cout << "  Solution nb " << ++nb << ": (branching depth: " << solution.branching_depth << ")" << std::endl;
                            }
                            output_solution_grid(std::cout, solution.grid, 2);
                            std::cout << std::endl;
                            return nb < max_nb_solutions;
                        };

                        /* Solve the grid */
                        picross::Solver::Status solver_status;
                        {
                            DurationMeas<float, std::milli> meas_ms(time_ms);
                            solver_status = solver->solve(input_grid, solution_found);
                        }

                        switch (solver_status)
                        {
                        case picross::Solver::Status::OK:
                            break;
                        case picross::Solver::Status::ABORTED:
                            if (nb == max_nb_solutions)
                                std::cout << "  Reached max number of solutions" << std::endl;
                            else
                                std::cout << "  Solver aborted" << std::endl;
                            std::cout << std::endl;
                            break;
                        case picross::Solver::Status::CONTRADICTORY_GRID:
                            std::cout << "  Not solvable" <<  std::endl;
                            std::cout << std::endl;
                            break;
                        case picross::Solver::Status::NOT_LINE_SOLVABLE:
                            std::cout << "  Not line solvable" << std::endl;
                            std::cout << std::endl;
                            break;
                        default:
                            assert(0);
                            break;
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
                    std::cout << "EXCPT [" << file_data.filename << "][" << input_grid.name() << "]: " << e.what() << std::endl;
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


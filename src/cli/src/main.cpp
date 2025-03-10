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

#include <stdutils/chrono.h>
#include <stdutils/platform.h>
#include <stdutils/string.h>
#include <utils/console_observer.h>
#include <utils/console_progress_observer.h>
#include <utils/input_grid_utils.h>
#include <utils/picross_file_io.h>

#include "argagg_wrap.h"

#include <cassert>
#include <chrono>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>


namespace {

    struct ValidationModeData
    {
        ValidationModeData()
            : filename()
            , gridname()
            , size()
            , validation_result()
            , timing_ms(-1.f)
            , grid_stats()
            , misc()
        {}

        std::string filename;
        std::string gridname;
        std::string size;
        picross::ValidationResult validation_result;
        float timing_ms;
        std::optional<picross::GridStats> grid_stats;
        std::string misc;
    };

    void stream_out_validation_mode_header(std::ostream& out, bool verbose)
    {
        static const std::vector<std::string> fields =
            { "File", "Grid", "Size", "Valid", "Difficulty", "Solutions", "Timing (ms)", "Misc",
              "Linear reductions", "Full reductions", "Min depth", "Max depth", "Searched line alternatives" };

        out << fields.at(0);
        const std::size_t last_idx = verbose ? fields.size() : 8;
        for (std::size_t idx = 1; idx < last_idx; idx++)
            out << ',' << fields.at(idx);
        out << std::endl;
    }

    std::ostream& operator<<(std::ostream& out, const ValidationModeData& data)
    {
        const auto found_solutions = static_cast<unsigned int>(std::max(0, data.validation_result.validation_code));
        assert(!data.grid_stats || data.grid_stats->nb_solutions == found_solutions);
        out << data.filename << ',';
        out << data.gridname << ',';
        out << data.size << ',';
        out << picross::str_validation_code(data.validation_result.validation_code) << ',';
        out << picross::str_difficulty_code(data.validation_result.difficulty_code) << ',';
        out << found_solutions << ',';
        if (data.timing_ms >= 0.f)
            out << data.timing_ms;
        out << ',';
        if (!data.validation_result.msg.empty() || !data.misc.empty())
            out << '"' << (data.misc.empty() ? data.validation_result.msg : data.misc) << '"';
        if (data.grid_stats.has_value())
        {
            out << ',' << (data.grid_stats->nb_single_line_linear_reduction + data.grid_stats->nb_single_line_linear_reduction_w_change);
            out << ',' << (data.grid_stats->nb_single_line_full_reduction   + data.grid_stats->nb_single_line_full_reduction_w_change);
            out << ',' << data.validation_result.branching_depth;
            out << ',' << data.grid_stats->max_branching_depth;
            out << ',' << (data.grid_stats->total_nb_branching_alternatives + data.grid_stats->total_nb_probing_alternatives);
            assert(picross::difficulty_code(data.grid_stats.value()) == data.validation_result.difficulty_code);
        }
        return out;
    }

    const stdutils::string::Indent CLI_INDENT(2);

    void output_solution_grid(std::ostream& out, const picross::OutputGrid& grid, unsigned int indentation_level = 0)
    {
        for (unsigned int y = 0u; y < grid.height(); y++)
        {
            out << CLI_INDENT(indentation_level) << grid.get_line<picross::Line::ROW>(y) << std::endl;
        }
    }

    void output_solution_stats(std::ostream& out, const picross::GridStats& stats, unsigned int indentation_level = 0)
    {
        std::stringstream buf;
        buf << stats;
        buf.seekg(0);
        for (std::string line; std::getline(buf, line);)
        {
            out << CLI_INDENT(line.empty() ? 0 : indentation_level) << line << std::endl;
        }
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
        "platform", { "--platform" },
        "Print platform information and exit", 0 },
      {
        "no-solution", { "--no-solution" },
        "Do not print out the solutions", 0 },
      {
        "no-stats", { "--no-stats" },
        "Do not print out the stats", 0 },
      {
        "no-timing", { "--no-timing" },
        "Do not print out timing measurements", 0 },
      {
        "quiet", { "--quiet" },
        "Disable most of the print out", 0 },
      {
        "verbose", { "-v", "--verbose" },
        "Print additional debug information", 0 },
      {
        "progress", { "--progress" },
        "Print branching progress in percent", 0 },
      {
        "max-nb-solutions", { "--max-nb-solutions" },
        "Limit the number of solutions returned per grid. Zero means no limit.", 1 },
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
        "The input is a text file with an output grid", 0 },
      {
        "output", { "--output" },
        "Output a grid file which format is deduced from the file extension", 1 }
    } };

    std::ostringstream usage_note;
    usage_note << "Picross Solver " << picross::get_version_string() << std::endl;
    usage_note << std::endl;
    usage_note << "Usage:" << std::endl;
    usage_note << "    picross_solver_cli [options] FILES" << std::endl;
    usage_note << std::endl;
    usage_note << "    Input FILES are grid definitions in one of the following formats: NON, NIN, PBM or native." << std::endl;
    usage_note << "    The file format is deduced from the file extension." << std::endl;
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
        stdutils::platform::print_compiler_all_info(std::cerr);
        exit(0);
    }

    if (args["platform"])
    {
        stdutils::platform::print_platform_info(std::cout);
        exit(0);
    }

    const bool validation_mode = args["validation-mode"];
    const std::chrono::seconds timeout_duration(args["timeout"].as<unsigned int>(0u));

    const bool quiet_mode         = args["quiet"];
    const bool verbose_mode       = !quiet_mode && args["verbose"];
    const bool print_out_grid_sz  = !quiet_mode;
    const bool print_out_solution = !quiet_mode && !args["no-solution"];
    const bool print_out_stats    = !quiet_mode && !args["no-stats"];
    const bool print_out_timing   = !quiet_mode && !args["no-timing"];

    // Depending on context, the default value for max_nb_solutions is different:
    //  - In validation mode, the default is 2
    //  - Otherwise is is zero, meaning no limit on the number of solutions.
    const unsigned int max_nb_solutions = args["max-nb-solutions"].as<unsigned int>(validation_mode ? 2 : 0);

    // Positional arguments
    if (args.pos.empty())
    {
        std::cerr << usage_note.str();
        exit(1);
    }

    // Output file (if set)
    const auto output_filepath = args["output"].as<std::string>("");
    const auto output_file_format = picross::io::picross_file_format_from_file_extension(output_filepath);

    int return_status = 0;
    unsigned int count_grids = 0u;

    if (validation_mode) { stream_out_validation_mode_header(std::cout, verbose_mode); }

    /* Solver */
    const auto solver = args["line-solver"] ? picross::get_line_solver() : picross::get_ref_solver();


    /***************************************************************************
     * II - Parse input files
     **************************************************************************/
    for (const char* filepath : args.pos)
    {
        ValidationModeData file_data;
        file_data.filename = std::filesystem::path(filepath).filename().string();

        const picross::io::ErrorHandler err_handler_classic = [&return_status, &file_data](picross::io::ErrorCodeT code, std::string_view msg)
        {
            std::cout << picross::io::str_error_code(code) << " [" << file_data.filename << "]: " << msg << std::endl;
            return_status = code;
        };
        const picross::io::ErrorHandler err_handler_validation = [&file_data](picross::io::ErrorCodeT code, std::string_view msg)
        {
            std::ostringstream oss;
            oss << picross::io::str_error_code(code) << " " << msg;
            file_data.misc = oss.str();
        };

        const auto format = [&args, &filepath]() -> picross::io::PicrossFileFormat {
            if (args["from_output"])
                return picross::io::PicrossFileFormat::OutputGrid;
            else
                return picross::io::picross_file_format_from_file_extension(filepath);
        }();

        const auto grids_to_solve = picross::io::parse_picross_file(filepath, format, (validation_mode ? err_handler_validation : err_handler_classic));;

        if (validation_mode && !file_data.misc.empty()) { std::cout << file_data << std::endl; }

        /***************************************************************************
         * III - Solve Picross puzzles
         **************************************************************************/
        for (const auto& io_grid : grids_to_solve)
        {
            const picross::InputGrid& input_grid = io_grid.m_input_grid;
            ValidationModeData grid_data = file_data;
            grid_data.gridname = input_grid.name();
            grid_data.size = picross::str_input_grid_size(input_grid);

            try
            {
                if (!validation_mode)
                {
                    std::cout << "GRID " << ++count_grids << ": " << input_grid.name() << std::endl;
                    if (print_out_grid_sz)
                    {
                        std::cout << CLI_INDENT << "Size: " << grid_data.size << std::endl;
                    }
                }

                /* Sanity check of the input data */
                const auto [input_ok, check_msg] = picross::check_input_grid(input_grid);
                grid_data.validation_result.validation_code = input_ok ? 0 : -1;
                grid_data.misc = check_msg;

                if (input_ok)
                {
                    if (verbose_mode && !validation_mode)
                    {
                        stream_input_grid_constraints(std::cout, input_grid);
                    }

                    /* Set observer */
                    const auto width = input_grid.width();
                    const auto height = input_grid.height();
                    ConsoleObserver obs(width, height, std::cout);
                    const auto& goal = io_grid.m_goal;
                    if (goal.has_value())
                    {
                        obs.verify_against_goal(*goal);
                    }
                    ConsoleProgressObserver progress_obs(std::cout);
                    if (!validation_mode)
                    {
                        if (verbose_mode)
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
                        stdutils::chrono::Timeout<std::chrono::seconds> timeout_clock(timeout_duration);
                        solver->set_abort_function([&timeout_clock]() { return timeout_clock.has_expired(); });
                    }

                    std::chrono::duration<float, std::milli> time_ms;
                    if (validation_mode)
                    {
                        /* Stats */
                        picross::GridStats stats;
                        if (verbose_mode)
                            solver->set_stats(stats);

                        /* Validate the grid */
                        {
                            stdutils::chrono::DurationMeas<float, std::milli> meas_ms(time_ms);
                            grid_data.validation_result = picross::validate_input_grid(*solver, input_grid, max_nb_solutions);
                        }
                        if (print_out_timing)
                            grid_data.timing_ms = time_ms.count();
                        if (verbose_mode)
                            grid_data.grid_stats = stats;
                    }
                    else
                    {
                        /* Stats */
                        picross::GridStats stats;
                        solver->set_stats(stats);

                        /* Output grid with a solution (in case the --output option is set) */
                        auto output_grid = io_grid;

                        /* Solution display */
                        unsigned int nb_solutions = 0;
                        picross::Solver::SolutionFound solution_found = [&nb_solutions, max_nb_solutions, &output_grid, print_out_solution](picross::Solver::Solution&& solution)
                        {
                            std::stringstream out_hdr;
                            if (solution.partial)
                            {
                                assert(!solution.grid.is_completed());
                                out_hdr << CLI_INDENT << "Partial solution:";
                            }
                            else
                            {
                                assert(solution.grid.is_completed());
                                if (!output_grid.m_goal.has_value())
                                {
                                    output_grid.m_goal = std::make_optional<picross::OutputGrid>(solution.grid);
                                }
                                out_hdr << CLI_INDENT << "Solution nb " << ++nb_solutions << ": (branching depth: " << solution.branching_depth << ")";
                            }
                            if (print_out_solution)
                            {
                                std::cout << out_hdr.str() << std::endl;
                                output_solution_grid(std::cout, solution.grid, 1);
                                std::cout << std::endl;
                            }
                            return max_nb_solutions == 0 || nb_solutions < max_nb_solutions;
                        };

                        /* Solve the grid */
                        picross::Solver::Status solver_status;
                        {
                            stdutils::chrono::DurationMeas<float, std::milli> meas_ms(time_ms);
                            solver_status = solver->solve(input_grid, solution_found);
                        }

                        switch (solver_status)
                        {
                        case picross::Solver::Status::OK:
                            if (!output_filepath.empty())
                            {
                                if (picross::io::save_picross_file(output_filepath, output_file_format, output_grid, err_handler_classic))
                                    std::cout << CLI_INDENT << "Saved file " << output_filepath << " (format: " << output_file_format << ")" << std::endl << std::endl;
                            }
                            break;
                        case picross::Solver::Status::ABORTED:
                            if (max_nb_solutions != 0 && nb_solutions == max_nb_solutions)
                                std::cout << CLI_INDENT << "Reached max number of solutions" << std::endl << std::endl;
                            else
                                std::cout << CLI_INDENT << "Solver aborted" << std::endl << std::endl;
                            break;
                        case picross::Solver::Status::CONTRADICTORY_GRID:
                            std::cout << CLI_INDENT << "Not solvable" << std::endl <<  std::endl;
                            break;
                        case picross::Solver::Status::NOT_LINE_SOLVABLE:
                            std::cout << CLI_INDENT << "Not line solvable" << std::endl << std::endl;
                            break;
                        default:
                            assert(0);
                            break;
                        }

                        /* Display stats */
                        if (print_out_stats)
                        {
                            output_solution_stats(std::cout, stats, 1);
                            std::cout << std::endl;
                        }

                        /* Display timings */
                        if (print_out_timing)
                        {
                            std::cout << CLI_INDENT << "Wall time: " << time_ms.count() << "ms" << std::endl;
                        }
                    }
                }
                else if (!validation_mode)
                {
                    std::cout << CLI_INDENT << "Invalid grid. Error message: " << grid_data.misc << std::endl;
                }
            }
            catch (std::exception& e)
            {
                if (validation_mode)
                {
                    grid_data.validation_result.validation_code = -1;   // ERR
                    grid_data.misc = "EXCEPTION";
                }
                else
                {
                    std::cout << "EXCEPTION [" << file_data.filename << "][" << input_grid.name() << "]: " << e.what() << std::endl;
                    return_status = 5;
                }
            }

            if (validation_mode)
            {
                std::cout << grid_data << std::endl;
            }
            else if (!quiet_mode)
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


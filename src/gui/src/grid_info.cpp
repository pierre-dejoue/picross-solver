#include "grid_info.h"

#include <picross/picross.h>
#include <stdutils/string.h>
#include <utils/input_grid_utils.h>

#include <imgui.h>

#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>


namespace
{
    constexpr std::string_view GRID_INFO = "Grid Info";
    constexpr std::string_view SOLVER_STATS = "Solver Stats";
    constexpr std::string_view GRID_CONSTRAINTS = "Grid Constraints";
    constexpr std::string_view GRID_ROWS = "Rows";
    constexpr std::string_view GRID_COLS = "Columns";

    enum class GridInfoSection
    {
        None        = 0,
        Basic       = 1 << 0,
        Stats       = 1 << 1,
        Constraints = 1 << 2
    };
}

GridInfo::GridInfo(const picross::InputGrid& grid)
    : grid(grid)
    , title(std::string(grid.name()) + " Info")
    , grid_metadata()
    , solver_stats()
    , solver_stats_mutex()
    , solver_stats_flag()
    , solver_stats_info()
{
    grid_metadata.emplace_back("Name", grid.name());
    grid_metadata.emplace_back("Size", picross::str_input_grid_size(grid));
    for (const auto&[key, data] : grid.metadata())
    {
        grid_metadata.emplace_back(stdutils::string::capitalize(key), data);
    }
}

void GridInfo::visit(bool& can_be_erased)
{
    constexpr ImGuiWindowFlags win_flags = ImGuiWindowFlags_AlwaysAutoResize;
    bool is_window_open = true;
    ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, 900));
    if (!ImGui::Begin(title.c_str(), &is_window_open, win_flags))
    {
        // Collapsed
        can_be_erased = !is_window_open;
        ImGui::End();
        return;
    }
    can_be_erased = !is_window_open;

    // Stats
    if (solver_stats_flag)
    {
        refresh_stats_info();
    }

    unsigned int active_sections = 0;
    const ImVec2 cell_padding(7, 4);
    constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

    // Copy button
    const bool copy_to_clipboard = ImGui::Button("Copy");

    // Grid basic info
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode(GRID_INFO.data()))
    {
        active_sections |= static_cast<unsigned int>(GridInfoSection::Basic);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
        if (ImGui::BeginTable("table_info", 2, table_flags))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("data", ImGuiTableColumnFlags_WidthStretch);
            //ImGui::TableHeadersRow();
            for (const auto& [key, data] : grid_metadata)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(key.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(data.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        ImGui::TreePop();
    }

    // Solver stats
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode(SOLVER_STATS.data()))
    {
        active_sections |= static_cast<unsigned int>(GridInfoSection::Stats);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
        if (ImGui::BeginTable("table_stats", 2, table_flags))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("data", ImGuiTableColumnFlags_WidthStretch);
            //ImGui::TableHeadersRow();
            for (const auto& [key, data] : solver_stats_info)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(key.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(data.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        ImGui::TreePop();
    }

    // Grid constraints
    if (ImGui::TreeNode(GRID_CONSTRAINTS.data()))
    {
        active_sections |= static_cast<unsigned int>(GridInfoSection::Constraints);

        const auto build_table_of_constraints = [&cell_padding, &table_flags, this](picross::Line::Type type, std::size_t size, const char* table_id)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
            if (ImGui::BeginTable(table_id, 3, table_flags))
            {
                ImGui::TableSetupColumn("Line",       ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
                ImGui::TableSetupColumn("",           ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
                ImGui::TableSetupColumn("Constraint", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                for (picross::Line::Index idx = 0; idx < size; idx++)
                {
                    std::stringstream line_id_ss;
                    picross::LineId line_id(type, idx);
                    line_id_ss << line_id;
                    std::stringstream constraint_ss;
                    // constraint_ss << "[ ";
                    picross::stream_input_grid_constraint(constraint_ss, grid, line_id);
                    // constraint_ss << " ]";
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(line_id_ss.str().c_str());
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(constraint_ss.str().c_str());
                }
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        };

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(GRID_ROWS.data()))
        {
            build_table_of_constraints(picross::Line::ROW, grid.height(), "table_rows");
            ImGui::TreePop();
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(GRID_COLS.data()))
        {
            build_table_of_constraints(picross::Line::COL, grid.width(), "table_cols");
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
    ImGui::End();

    // If requested, copy raw text to clipobard
    if (copy_to_clipboard)
    {
        ImGuiIO io;
        const std::string cc = info_as_string(active_sections);
        io.SetClipboardTextFn(io.ClipboardUserData, cc.c_str());
    }
}

void GridInfo::update_solver_status(unsigned int nb_found_solutions, unsigned int current_depth, float progress)
{
    std::lock_guard<std::mutex> lock(solver_stats_mutex);
    solver_stats.m_ongoing = true;
    solver_stats.m_nb_solutions = nb_found_solutions;
    solver_stats.m_current_depth = current_depth;
    solver_stats.m_progress = progress;
    solver_stats_flag = true;
}

void GridInfo::solver_completed(const picross::GridStats& stats)
{
    std::lock_guard<std::mutex> lock(solver_stats_mutex);
    solver_stats.m_ongoing = false;
    solver_stats.m_grid_stats = stats;
    solver_stats.m_nb_solutions = stats.nb_solutions;
    solver_stats.m_current_depth = 0u;
    solver_stats.m_progress = 1.f;
    solver_stats_flag = true;
}

void GridInfo::refresh_stats_info()
{
    std::lock_guard<std::mutex> lock(solver_stats_mutex);
    solver_stats_info.clear();
    solver_stats_info.emplace_back("Solving", solver_stats.m_ongoing ? "ONGOING" : "DONE");
    if (!solver_stats.m_ongoing)
    {
        std::string_view difficulty = picross::str_difficulty_code(difficulty_code(solver_stats.m_grid_stats));
        solver_stats_info.emplace_back("Difficulty", difficulty);
    }
    solver_stats_info.emplace_back("Nb solutions", std::to_string(solver_stats.m_nb_solutions));
    if (solver_stats.m_ongoing)
    {
        solver_stats_info.emplace_back("Current depth", std::to_string(solver_stats.m_current_depth));
        solver_stats_info.emplace_back("Progress", std::to_string(solver_stats.m_progress));
    }
    else
    {
        solver_stats_info.emplace_back("Max depth", std::to_string(solver_stats.m_grid_stats.max_branching_depth));
    }
    solver_stats_flag = false;
}

namespace
{
    std::size_t key_col_width(const GridInfo::InfoMap& map)
    {
        std::size_t result = 0;
        for (const auto& kvp: map)
            result = std::max(result, kvp.first.size());
        result += 2;
        return result;
    }
}

std::string GridInfo::info_as_string(unsigned int active_sections) const
{
    std::stringstream out;

    // Basic grid info
    out << GRID_INFO;
    if (active_sections & static_cast<unsigned int>(GridInfoSection::Basic))
    {
        out << std::endl;
        const auto key_w = key_col_width(grid_metadata);
        for (const auto&[key, data]: grid_metadata)
            out << "  " << std::setw(key_w) << std::left << key << data << std::endl;
    }
    out << std::endl;

    // Solver stats
    out << SOLVER_STATS;
    if (active_sections & static_cast<unsigned int>(GridInfoSection::Stats))
    {
        out << std::endl;
        const auto key_w = key_col_width(solver_stats_info);
        for (const auto&[key, data]: solver_stats_info)
            out << "  " << std::setw(key_w) << std::left << key << data << std::endl;
    }
    out << std::endl;

    // Grid constraints
    out << GRID_CONSTRAINTS;
    if (active_sections & static_cast<unsigned int>(GridInfoSection::Constraints))
    {
        out << std::endl;
        const auto dump_table_of_constraints = [&out, this](picross::Line::Type type, std::size_t size, const char* section_name)
        {
            out << "  " << section_name << std::endl;
            for (picross::Line::Index idx = 0; idx < size; idx++)
            {
                out << "    ";
                picross::stream_input_grid_line_id_and_constraint(out, grid, picross::LineId(type, idx));
            }
        };
        dump_table_of_constraints(picross::Line::ROW, grid.height(), GRID_ROWS.data());
        dump_table_of_constraints(picross::Line::COL, grid.width(),  GRID_COLS.data());
    }
    out << std::endl;

    return out.str();
}

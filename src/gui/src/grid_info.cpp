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
    constexpr std::string_view GRID_CONSTRAINTS = "Grid Constraints";
    constexpr std::string_view GRID_ROWS = "Rows";
    constexpr std::string_view GRID_COLS = "Columns";

    enum class GridInfoSection
    {
        None        = 0,
        Basic       = 1 << 0,
        Constraints = 1 << 1
    };
}

GridInfo::GridInfo(const picross::InputGrid& grid)
    : grid(grid)
    , title(std::string(grid.name()) + " Info")
    , grid_metadata()
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

std::string GridInfo::info_as_string(unsigned int active_sections) const
{
    std::stringstream out;
    std::size_t key_col_width = 0;
    for (const auto& kvp: grid_metadata)
        key_col_width = std::max(key_col_width, kvp.first.size());
    key_col_width += 2;

    // Basic grid info
    out << GRID_INFO;
    if (active_sections & static_cast<unsigned int>(GridInfoSection::Basic))
    {
        out << std::endl;
        for (const auto&[key, data]: grid_metadata)
            out << "  " << std::setw(key_col_width) << std::left << key << data << std::endl;
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

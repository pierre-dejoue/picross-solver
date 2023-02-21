/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - Definition of the input puzzle
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace picross
{

/*
 * InputGrid class
 *
 *   A data structure to store the information related to an unsolved Picross grid.
 *   This is basically the input information of the solver.
 */
class InputGrid
{
public:
    // Constraint that applies to a line (row or column)
    // It gives the size of each group of contiguous filled tiles
    using Constraint = std::vector<unsigned int>;
    using Constraints = std::vector<Constraint>;

    // Optional grid metadata
    using Metadata = std::map<std::string, std::string>;

    InputGrid(const Constraints& rows, const Constraints& cols, const std::string_view name = "");
    InputGrid(Constraints&& rows, Constraints&& cols, const std::string_view name = "");

    std::size_t width() const { return m_cols.size(); }
    std::size_t height() const { return m_rows.size(); }
    const Constraints& rows() const { return m_rows; }
    const Constraints& cols() const { return m_cols; }
    std::string_view name() const { return m_name; }
    const Metadata& metadata() const { return m_metadata; }

    void set_name(const std::string_view name);
    void set_metadata(std::string_view key, std::string_view data);

private:
    Constraints     m_rows;
    Constraints     m_cols;
    std::string     m_name;
    Metadata        m_metadata;
};


/*
 * Return the grid size as a string "WxH" with W the width and H the height
 */
std::string str_input_grid_size(const InputGrid& grid);


/*
 * Sanity check of the constraints of the input grid:
 *
 * - Non-zero height and width
 * - Same number of filled tiles on the rows and columns
 * - The height and width are sufficient to cope with the individual constraints
 */
std::pair<bool, std::string> check_input_grid(const InputGrid& grid);

} // namespace picross

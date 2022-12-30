/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <picross/picross.h>


namespace picross
{
/*
 * Line Identifier
 */
struct LineId
{
    LineId(Line::Type type = Line::ROW, Line::Index index = 0u)
        : m_type(type), m_index(index)
    {}

    Line::Type  m_type;
    Line::Index m_index;
};

/*
 * Line related functions
 */
bool is_all_one_color(const Line& line, Tile color);
bool is_complete(const Line& line);
std::string str_line(const Line& line);
std::string str_line_type(Line::Type type);
Line line_delta(const Line& lhs, const Line& rhs);
InputGrid::Constraint get_constraint_from(const Line& line);
Line operator+(const Line& lhs, const Line& rhs);

} // namespace picross

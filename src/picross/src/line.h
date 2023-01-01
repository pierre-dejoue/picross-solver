/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include "line_span.h"

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
Line operator+(const LineSpan& lhs, const LineSpan& rhs);
Line operator-(const LineSpan& lhs, const LineSpan& rhs);
void line_reduce(Line& lhs, const LineSpan& rhs);
bool is_line_uniform(const LineSpan& line, Tile color);
bool is_line_complete(const LineSpan& line);
Line line_from_line_span(const LineSpan& line_span);
void copy_line_from_line_span(Line& line, const LineSpan& line_span);
InputGrid::Constraint get_constraint_from(const LineSpan& line_span);

} // namespace picross

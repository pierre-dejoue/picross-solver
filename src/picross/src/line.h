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

#include <cassert>


namespace picross
{

/*
 * Line range
 *
 * A range of rows or column that satisfy some criteria (for example, the range of uncompleted rows).
 * The elements in the range are indices: m_begin, m_begin + 1, ..., m_end - 1.
 * The range can be empty (i.e. m_begin == m_end), meaning now index satisfies the criteria
 */
struct LineRange
{
    Line::Index m_begin = 0u;
    Line::Index m_end = 0u;

    bool empty() const { assert(m_begin <= m_end); return m_begin == m_end; }
    Line::Index first() const { assert(!empty()); return m_begin; }
    Line::Index last() const { assert(!empty()); return m_end - 1u; }
};

/*
 * Line related functions
 */
Line operator+(const LineSpan& lhs, const LineSpan& rhs);
Line operator-(const LineSpan& lhs, const LineSpan& rhs);
bool is_line_uniform(const LineSpan& line, Tile color);
Line line_from_line_span(const LineSpan& line_span);
InputGrid::Constraint get_constraint_from(const LineSpan& line_span);

template <typename TileT>
void copy_line_span(LineSpanW& target, const LineSpanImpl<TileT>& source)
{
    assert(target.type() == source.type());
    assert(target.index() == source.index());
    assert(target.size() == source.size());
    const int line_sz = static_cast<int>(source.size());
    for (int idx = 0; idx < line_sz; idx++)
        target[idx] = source[idx];
}

template <typename TileT>
void copy_line_span(Line& target, const LineSpanImpl<TileT>& source)
{
    LineSpanW target_span(target);
    copy_line_span(target_span, source);
}

} // namespace picross

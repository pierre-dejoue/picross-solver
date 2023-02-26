/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Line cache used during search
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include "grid.h"
#include "line_alternatives.h"
#include "line_span.h"

#include <picross/picross.h>

#include <cstddef>
#include <memory>

namespace picross
{

class LineCache
{
public:
    LineCache(std::size_t width = 0u, std::size_t height = 0u);
    LineCache(const LineCache&) = delete;
    LineCache(LineCache&&) noexcept;
    LineCache& operator=(const LineCache&) = delete;
    LineCache& operator=(LineCache&&) noexcept;
    ~LineCache();

    struct Entry
    {
        LineSpan                m_line_span;
        LineAlternatives::NbAlt m_nb_alt;
    };
    static bool is_valid(const Entry& entry);
    Entry read_line(LineId line_id, Tile key) const;
    void store_line(LineId line_id, Tile key, const LineSpan& line, LineAlternatives::NbAlt nb_alt);
    void clear();

private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace picross
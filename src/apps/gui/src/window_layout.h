#pragma once

#include "screen.h"

#include <algorithm>
#include <cassert>


struct WindowLayout
{
    static constexpr float DEFAULT_PADDING = 2.0f;

    // A size of 0.f or less on an axis means the window will occupy whatever is left of the workspace on that axis
    WindowLayout(float pos_x = 0.f, float pos_y = 0.f, float sz_x = 0.f, float sz_y = 0.f, float padding = DEFAULT_PADDING)
        : m_position(pos_x + padding, pos_y + padding)
        , m_size(sz_x, sz_y)
        , m_padding(padding)
    {
        assert(m_position.x >= 0.f);
        assert(m_position.y >= 0.f);
    }

    ScreenSize window_size(ScreenSize workspace_sz) const
    {
        return ScreenSize(
            std::max(1.f, m_size.x > 0.f ? m_size.x - m_padding : workspace_sz.x - m_position.x - m_padding),
            std::max(1.f, m_size.y > 0.f ? m_size.y - m_padding : workspace_sz.y - m_position.y - m_padding)
        );
    }

    ScreenPos m_position;
    ScreenSize m_size;
    float m_padding;
};

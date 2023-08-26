#pragma once

template <typename F>
struct Vec2
{
    using scalar = F;
    Vec2() : x(F(0)), y(F(0)) {}
    Vec2(F x, F y) : x(x), y(y) {}
    F x, y;
};

using ScreenPos = Vec2<float>;
using ScreenSize = Vec2<float>;

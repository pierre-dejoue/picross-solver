#pragma once

#include <algorithm>


struct Parameter
{
    template <typename T>
    struct Limits
    {
        T def;
        T min;
        T max;

        void clamp(T& v) const { v = std::max(min, std::min(v, max)); }
    };
    static constexpr Limits<bool> limits_true { true, false, true };
    static constexpr Limits<bool> limits_false { false, false, true };
};

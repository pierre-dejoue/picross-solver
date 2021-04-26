#pragma once

#include <chrono>

template<typename Rep, typename Period>
class DurationMeas
{
public:
    explicit DurationMeas(std::chrono::duration<Rep, Period>& out_duration) : out_duration(out_duration)
    {
        start = std::chrono::steady_clock::now();
    }

    ~DurationMeas()
    {
        const auto end = std::chrono::steady_clock::now();
        out_duration = end - start;
    }

private:
    std::chrono::duration<Rep, Period>& out_duration;
    std::chrono::time_point<std::chrono::steady_clock> start;
};

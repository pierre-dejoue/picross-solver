// Copyright (c) 2021 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <chrono>

namespace stdutils {
namespace chrono {

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

template <typename Duration>
class Timeout
{
    using time_point = std::chrono::time_point<std::chrono::steady_clock, Duration>;
public:
    Timeout(Duration duration)
        : m_timeout_end(std::chrono::time_point_cast<Duration>(std::chrono::steady_clock::now()))
    {
        m_timeout_end += duration;
    }

    bool has_expired() const
    {
        return std::chrono::steady_clock::now() > m_timeout_end;
    }
private:
    time_point m_timeout_end;
};

} // namespace chrono
} // namespace stdutils

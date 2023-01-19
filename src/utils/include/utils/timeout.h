#pragma once

#include <chrono>

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

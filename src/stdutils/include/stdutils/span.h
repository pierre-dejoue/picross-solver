#pragma once

#include <cassert>
#include <cstddef>

namespace stdutils
{

// Imitation of std::span in C++20
template <typename T>
class span
{
public:
    span(T* ptr, std::size_t size) noexcept : m_ptr(ptr), m_size(size) { assert(m_ptr); }
    span(const span<T>&) noexcept = default;
    span(span<T>&&) noexcept = default;
    span<T>& operator=(const span<T>&) noexcept = default;
    span<T>& operator=(span<T>&&) noexcept = default;

    // For qualification conversions (e.g. non-const T to const T)
    template <typename R>
    span(const span<R>& other) noexcept : m_ptr(other.begin()), m_size(other.size()) { assert(m_ptr); }

    std::size_t size() const { return m_size; }

    T* begin() { return m_ptr; }
    T* end() { return m_ptr + m_size; }
    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + m_size; }

    T& operator[](std::size_t idx) { assert(idx < m_size); return *(m_ptr + idx); }
    const T& operator[](std::size_t idx) const { assert(idx < m_size); return *(m_ptr + idx); }

private:
    T*          m_ptr;
    std::size_t m_size;
};

} // namespace stdutils
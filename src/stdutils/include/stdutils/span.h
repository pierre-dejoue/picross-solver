// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace stdutils {

inline constexpr std::size_t dyn_extent = std::numeric_limits<std::size_t>::max();  // -1

// Imitation of std::span in C++20

template <typename T, std::size_t Sz = dyn_extent, class Enable = void>
class span {};

template <typename T>
class span<T, dyn_extent, void>
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

    T* data() { return m_ptr; }
    T* begin() { return m_ptr; }
    T* end() { return m_ptr + m_size; }
    const T* data() const { return m_ptr; }
    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + m_size; }

    T& operator[](std::size_t idx) { assert(idx < m_size); return *(m_ptr + idx); }
    const T& operator[](std::size_t idx) const { assert(idx < m_size); return *(m_ptr + idx); }

private:
    T*          m_ptr;
    std::size_t m_size;
};

template <typename T, std::size_t Sz>
class span<T, Sz, std::enable_if_t<Sz != dyn_extent>>
{
    using this_type = span<T, Sz, std::enable_if_t<Sz != dyn_extent>>;
public:
    explicit span(T* ptr) noexcept : m_ptr(ptr) { assert(m_ptr); }
    span(const this_type&) noexcept = default;
    span(this_type&&) noexcept = default;
    this_type& operator=(const this_type&) noexcept = default;
    this_type& operator=(this_type&&) noexcept = default;

    // For qualification conversions (e.g. non-const T to const T)
    template <typename R>
    span(const span<R, Sz, void>& other) noexcept : m_ptr(other.begin()) { assert(m_ptr); }

    constexpr std::size_t size() const { return Sz; }

    T* data() { return m_ptr; }
    T* begin() { return m_ptr; }
    T* end() { return m_ptr + Sz; }
    const T* data() const { return m_ptr; }
    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + Sz; }

    T& operator[](std::size_t idx) { assert(idx < Sz); return *(m_ptr + idx); }
    const T& operator[](std::size_t idx) const { assert(idx < Sz); return *(m_ptr + idx); }

private:
    T* m_ptr;
};

} // namespace stdutils

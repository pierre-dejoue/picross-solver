// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace stdutils {

inline constexpr std::size_t dyn_extent = std::numeric_limits<std::size_t>::max();  // -1

// Imitation of std::span in C++20

template <typename T, std::size_t Sz = dyn_extent, class Enable = void>
class Span {};

template <typename T>
class Span<T, dyn_extent, void>
{
public:
    using element_type = T;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

public:
    Span() noexcept : m_ptr(nullptr), m_size(0) { }
    Span(T* ptr, std::size_t size) noexcept : m_ptr(ptr), m_size(size) { assert(m_ptr); }
    Span(const Span<T>&) noexcept = default;
    Span(Span<T>&&) noexcept = default;
    Span<T>& operator=(const Span<T>&) noexcept = default;
    Span<T>& operator=(Span<T>&&) noexcept = default;

    // For qualification conversions (e.g. non-const T to const T)
    template <typename R>
    Span(const Span<R>& other) noexcept : m_ptr(other.data()), m_size(other.size()) { assert(m_ptr); }

    std::size_t size() const  noexcept { return m_size; }
    bool empty() const noexcept { return m_size == 0 || m_ptr == nullptr; }

    pointer data() noexcept { return m_ptr; }
    pointer begin() noexcept { return m_ptr; }
    pointer end() noexcept { return m_ptr + m_size; }
    const_pointer data() const noexcept { return m_ptr; }
    const_pointer begin() const noexcept { return m_ptr; }
    const_pointer end() const noexcept { return m_ptr + m_size; }

    reference operator[](std::size_t idx) { assert(idx < m_size); return *(m_ptr + idx); }
    const_reference operator[](std::size_t idx) const { assert(idx < m_size); return *(m_ptr + idx); }

private:
    T*          m_ptr;
    std::size_t m_size;
};

template <typename C>
Span<typename C::value_type, dyn_extent> make_span(C& container)
{
    using T = typename C::value_type;
    return Span<T, dyn_extent>(container.data(), container.size());
}

template <typename C>
Span<const typename C::value_type, dyn_extent> make_const_span(const C& container)
{
    using T = typename C::value_type;
    return Span<const T, dyn_extent>(container.data(), container.size());
}

template <typename T, std::size_t Sz>
class Span<T, Sz, std::enable_if_t<Sz != dyn_extent>>
{
    using this_type = Span<T, Sz, std::enable_if_t<Sz != dyn_extent>>;
    static_assert(Sz > 0);

public:
    using element_type = T;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

public:
    explicit Span(T* ptr) noexcept : m_ptr(ptr) { assert(m_ptr); }
    explicit Span(std::array<T, Sz>& arr) noexcept : m_ptr(arr.data()) { assert(m_ptr); }
    explicit Span(const std::array<std::remove_const_t<T>, Sz>& arr) noexcept : m_ptr(arr.data()) { assert(m_ptr); }
    Span(const this_type&) noexcept = default;
    Span(this_type&&) noexcept = default;
    this_type& operator=(const this_type&) noexcept = default;
    this_type& operator=(this_type&&) noexcept = default;

    // For qualification conversions (e.g. non-const T to const T)
    template <typename R>
    Span(const Span<R, Sz, void>& other) noexcept : m_ptr(other.begin()) { assert(m_ptr); }

    constexpr std::size_t size() const noexcept { return Sz; }

    pointer data() noexcept { return m_ptr; }
    pointer begin() noexcept { return m_ptr; }
    pointer end() noexcept { return m_ptr + Sz; }
    const_pointer data() const noexcept { return m_ptr; }
    const_pointer begin() const noexcept { return m_ptr; }
    const_pointer end() const noexcept { return m_ptr + Sz; }

    reference operator[](std::size_t idx) { assert(idx < Sz); return *(m_ptr + idx); }
    const_reference operator[](std::size_t idx) const { assert(idx < Sz); return *(m_ptr + idx); }

private:
    T* m_ptr;
};

} // namespace stdutils

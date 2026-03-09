// Copyright (c) 2025 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace stdutils {

/**
 * Generic trait to retrieve the constexpr size N of an array (Either of type T[N] or std::array<T, N>)
 *
 * Inspiration from https://stackoverflow.com/questions/22712965/using-stdextent-on-stdarray
 */
template<typename T>
struct array_size : std::extent<T> { };
template<typename T, size_t N>
struct array_size<std::array<T,N>> : std::tuple_size<std::array<T,N>> { };

template<typename T>
constexpr std::size_t array_size_v = array_size<T>::value;

} // namespace stdutils

// Copyright (c) 2024 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <stdutils/macros.h>
#include <stdutils/span.h>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace stdutils {

/**
 * Versions of memcpy with sanity checks
 *
 * - Check for null pointers
 * - Check that nb_bytes <= max_dest_sz * sizeof(T)
 * - In case of error, copy nothing and return false
 */
template <typename T>
bool memcpy(T* dest, const void* src);                                                   // Copies at most sizeof(T) bytes

template <typename T>
bool memcpy(T* dest, std::size_t max_dest_sz, const void* src, std::size_t nb_bytes);    // Copies at most max_dest_sz * sizeof(T) bytes


/**
 * Versions of memset with sanity checks
 */
template <typename T>
bool memset(T* dest, int ch);                                                            // Sets at most sizeof(T) bytes

template <typename T>
bool memset(T* dest, std::size_t max_dest_sz, int ch, std::size_t nb_bytes);             // Sets at most max_dest_sz * sizeof(T) bytes


//
//
// Implementation
//
//


template <typename T>
bool memcpy(T* dest, const void* src)
{
    if (dest == nullptr || src == nullptr)
    {
        assert(0);
        return false;
    }
    IGNORE_RETURN std::memcpy(static_cast<void*>(dest), src, sizeof(T));
    return true;
}

template <typename T>
bool memcpy(T* dest, std::size_t max_dest_sz, const void* src, std::size_t nb_bytes)
{
    if (dest == nullptr || src == nullptr)
    {
        assert(0);
        return false;
    }
    if (nb_bytes > max_dest_sz * sizeof(T))
    {
        assert(0);
        return false;
    }
    IGNORE_RETURN std::memcpy(static_cast<void*>(dest), src, nb_bytes);
    return true;
}

template <typename T>
bool memset(T* dest, int ch)
{
    if (dest == nullptr)
    {
        assert(0);
        return false;
    }
    IGNORE_RETURN std::memset(static_cast<void*>(dest), ch, sizeof(T));
    return true;
}

template <typename T>
bool memset(T* dest, std::size_t max_dest_sz, int ch, std::size_t nb_bytes)
{
    if (dest == nullptr)
    {
        assert(0);
        return false;
    }
    if (nb_bytes > max_dest_sz * sizeof(T))
    {
        assert(0);
        return false;
    }
    IGNORE_RETURN std::memset(static_cast<void*>(dest), ch, nb_bytes);
    return true;
}

} // namespace stdutils

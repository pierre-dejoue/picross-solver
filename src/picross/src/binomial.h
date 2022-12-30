/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <cassert>
#include <limits>
#include <vector>

namespace picross
{
namespace BinomialCoefficients
{
using Rep = unsigned int;

/*
 * The max value returned for a number of alternatives (indicates overflow)
 */
constexpr Rep overflowValue() noexcept
{
    return std::numeric_limits<Rep>::max();
}

/*
 * Safely add number of alternatives, taking overflows into account
 */
constexpr Rep& add(Rep& lhs, const Rep rhs) noexcept
{
    Rep sum = lhs + rhs;
    if (sum >= lhs)
    {
        assert(sum >= rhs);
        lhs = sum;
    }
    else
    {
        assert(sum < rhs);
        lhs = overflowValue();
    }
    return lhs;
}

/*
 * A cache used to store already computed binomial numbers.
 *
 * This is to optimize the computation of the number of alternatives on empty lines.
 */
class Cache
{
public:
    Cache() = default;
    Cache(const Cache& other) = delete;
    Cache& operator=(const Cache& other) = delete;

    /*
     * Returns the number of ways to partition nb_elts into nb_buckets.
     *
     * - It is equal to the binomial coefficient (n k) with n = nb_elts + nb_buckets - 1  and k = nb_buckets - 1
     * - nb_buckets must be strictily positive
     * - Returns overflowValue() in case of overflow (which can happen rapidly!)
     */
    Rep partition_n_elts_into_k_buckets(unsigned int nb_elts, unsigned int nb_buckets);

private:
    std::vector<Rep> binomial_numbers;
};

} // namespace BinomialCoefficients
} // namespace picross

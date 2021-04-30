/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "binomial.h"

#include <cassert>
#include <limits>

namespace picross
{

BinomialCoefficientsCache::Rep BinomialCoefficientsCache::overflowValue()
{
    return std::numeric_limits<Rep>::max();
}


unsigned int BinomialCoefficientsCache::nb_alternatives_for_fixed_nb_of_partitions(unsigned int nb_elts, unsigned int nb_buckets)
{
    assert(nb_buckets > 0u);
    const unsigned int k = nb_buckets - 1;
    const unsigned int n = nb_elts + k;
    if (k == 0u || n == k)
    {
        return 1u;
    }
    assert(k >= 1u && n >= 2u);

    // Index in the binomial number cache
    const unsigned int idx = (n - 2)*(n - 1) / 2 + (k - 1);

    if (idx >= binomial_numbers.size())
    {
        binomial_numbers.resize(idx + 1, 0u);
    }

    auto& binomial_number = binomial_numbers[idx];
    if (binomial_number > 0u)
    {
        return binomial_numbers[idx];
    }
    else
    {
        static const Rep MAX = overflowValue();

        Rep accumulator = 0u;
        for (unsigned int e = 0u; e <= nb_elts; e++)
        {
            const Rep partial = nb_alternatives_for_fixed_nb_of_partitions(e, nb_buckets - 1u);
            if (accumulator > MAX - partial)
            {
                accumulator = MAX;
                break;
            }
            accumulator += partial;
        }
        binomial_number = accumulator;
        return accumulator;
    }
}

} // namespace picross

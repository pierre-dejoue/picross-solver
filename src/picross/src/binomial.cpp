/*******************************************************************************
 * PICROSS SOLVER
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#include "binomial.h"

#include <cstddef>

namespace picross {
namespace binomial {

Rep Cache::partition_n_elts_into_k_buckets(unsigned int nb_elts, unsigned int nb_buckets)
{
    assert(nb_buckets > 0u);
    const unsigned int k = nb_buckets - 1;
    const unsigned int n = nb_elts + k;
    if (k == 0u || n == k)
    {
        return Rep{1};
    }
    if (k == 1)
    {
        return Rep{n};
    }
    assert(k >= 2u && n >= 3u);

    // Index in the binomial number cache
    const std::size_t idx = (n - 3) * (n - 2) / 2 + (k - 2);

    if (idx >= binomial_numbers.size())
    {
        binomial_numbers.resize(idx + 1, Rep{0});
    }

    auto& binomial_number = binomial_numbers[idx];
    if (binomial_number == Rep{0})
    {
        constexpr Rep MAX = overflowValue();
        Rep accumulator{0};
        for (unsigned int e = 0u; e <= nb_elts; e++)
        {
            const Rep partial = partition_n_elts_into_k_buckets(e, nb_buckets - 1u);
            if (accumulator > MAX - partial)
            {
                accumulator = MAX;
                break;
            }
            accumulator += partial;
        }
        binomial_number = accumulator;
    }
    return binomial_number;
}

} // namespace binomial
} // namespace picross

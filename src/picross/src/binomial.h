/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <vector>

namespace picross
{

/*
 * Utility class used to store already computed binomial numbers.
 *
 * This is to optimize the computation of the number of alternatives on empty lines.
 */
class BinomialCoefficientsCache
{
public:
    using Rep = unsigned int;

    BinomialCoefficientsCache() = default;
    BinomialCoefficientsCache(const BinomialCoefficientsCache& other) = delete;
    BinomialCoefficientsCache& operator==(const BinomialCoefficientsCache& other) = delete;

    static Rep overflowValue();

    /*
     * Returns the number of ways to divide nb_elts into nb_buckets.
     *
     * Equal to the binomial coefficient (n k) with n = nb_elts + nb_buckets - 1  and k = nb_buckets - 1
     *
     * Returns std::numeric_limits<unsigned int>::max() in case of overflow (which happens rapidly)
     */
    Rep nb_alternatives_for_fixed_nb_of_partitions(unsigned int nb_cells, unsigned int nb_partitions);
private:
    std::vector<Rep> binomial_numbers;
};

} // namespace picross

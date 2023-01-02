#include <catch2/catch_test_macros.hpp>

#include "binomial.h"


namespace picross
{

TEST_CASE("partition_n_elts_into_k_buckets", "[binomial]")
{
    BinomialCoefficients::Cache binomial;
    CHECK(binomial.partition_n_elts_into_k_buckets(3, 1) == 1);
    CHECK(binomial.partition_n_elts_into_k_buckets(1, 2) == 2);
    CHECK(binomial.partition_n_elts_into_k_buckets(3, 2) == 4);
    CHECK(binomial.partition_n_elts_into_k_buckets(12, 4) == 455);
}

TEST_CASE("overflow", "[binomial]")
{
    BinomialCoefficients::Cache binomial;
    constexpr auto MAX = BinomialCoefficients::overflowValue();
    CHECK(binomial.partition_n_elts_into_k_buckets(24, 14) == 3562467300u);
    CHECK(binomial.partition_n_elts_into_k_buckets(23, 15) == MAX);
    CHECK(binomial.partition_n_elts_into_k_buckets(24, 15) == MAX);
}

TEST_CASE("add", "[binomial]")
{
    BinomialCoefficients::Cache binomial;
    {
        BinomialCoefficients::Rep nb_alternatives{0};
        BinomialCoefficients::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(3, 2));
        CHECK(nb_alternatives == 4);
        BinomialCoefficients::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(2, 2));
        CHECK(nb_alternatives == 7);
        BinomialCoefficients::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(1, 2));
        CHECK(nb_alternatives == 9);
        BinomialCoefficients::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(0, 2));
        CHECK(nb_alternatives == 10);
    }
    {
        constexpr auto MAX = BinomialCoefficients::overflowValue();
        BinomialCoefficients::Rep nb_alternatives{0};
        BinomialCoefficients::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(25, 13));
        CHECK(nb_alternatives == 1852482996u);
        BinomialCoefficients::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(25, 13));
        CHECK(nb_alternatives == 2u * 1852482996u);
        BinomialCoefficients::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(25, 13));
        CHECK(nb_alternatives == MAX);
        BinomialCoefficients::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(25, 13));
        CHECK(nb_alternatives == MAX);
    }
}

} // namespace picross

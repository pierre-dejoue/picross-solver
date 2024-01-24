#include <catch_amalgamated.hpp>

#include "binomial.h"


namespace picross {

TEST_CASE("partition_n_elts_into_k_buckets", "[binomial]")
{
    binomial::Cache binomial;
    CHECK(binomial.partition_n_elts_into_k_buckets(3, 1) == 1);
    CHECK(binomial.partition_n_elts_into_k_buckets(1, 2) == 2);
    CHECK(binomial.partition_n_elts_into_k_buckets(3, 2) == 4);
    CHECK(binomial.partition_n_elts_into_k_buckets(12, 4) == 455);
}

TEST_CASE("overflow", "[binomial]")
{
    binomial::Cache binomial;
    constexpr auto MAX = binomial::overflowValue();
    CHECK(binomial.partition_n_elts_into_k_buckets(24, 14) == 3562467300u);
    CHECK(binomial.partition_n_elts_into_k_buckets(23, 15) == MAX);
    CHECK(binomial.partition_n_elts_into_k_buckets(24, 15) == MAX);
}

TEST_CASE("add", "[binomial]")
{
    binomial::Cache binomial;
    {
        binomial::Rep nb_alternatives{0};
        binomial::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(3, 2));
        CHECK(nb_alternatives == 4);
        binomial::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(2, 2));
        CHECK(nb_alternatives == 7);
        binomial::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(1, 2));
        CHECK(nb_alternatives == 9);
        binomial::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(0, 2));
        CHECK(nb_alternatives == 10);
    }
    {
        constexpr auto MAX = binomial::overflowValue();
        binomial::Rep nb_alternatives{0};
        binomial::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(25, 13));
        CHECK(nb_alternatives == 1852482996u);
        binomial::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(25, 13));
        CHECK(nb_alternatives == 2u * 1852482996u);
        binomial::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(25, 13));
        CHECK(nb_alternatives == MAX);
        binomial::add(nb_alternatives, binomial.partition_n_elts_into_k_buckets(25, 13));
        CHECK(nb_alternatives == MAX);
    }
}

TEST_CASE("mult", "[binomial]")
{
    binomial::Cache binomial;
    {
        binomial::Rep nb_alternatives{1};
        binomial::mult(nb_alternatives, 5);
        CHECK(nb_alternatives == 5);
        binomial::mult(nb_alternatives, 7);
        CHECK(nb_alternatives == 35);
    }
    {
        constexpr auto MAX = binomial::overflowValue();
        binomial::Rep nb_alternatives{1};
        for (unsigned int count = 0; count < 16; count++)
             binomial::mult(nb_alternatives, 4);
        CHECK(nb_alternatives == MAX);
        binomial::mult(nb_alternatives, 3);
        CHECK(nb_alternatives == MAX);
    }
}

} // namespace picross

#pragma once

#include <picross/picross.h>

#include <cstddef>

namespace picross
{
namespace BinomialCoefficients { class Cache; }
class LineConstraint;
class LineSpan;

// Given a constraint, recursively build the possible alternatives of a Line and reduce them.
class LineAlternatives
{
public:
    using NbAlt = unsigned int;
public:
    LineAlternatives(const LineConstraint& constraint, const LineSpan& known_tiles, BinomialCoefficients::Cache& binomial);
    LineAlternatives(const LineAlternatives& other, const LineSpan& known_tiles);
    ~LineAlternatives();
    // Movable
    LineAlternatives(LineAlternatives&&) noexcept;
    LineAlternatives& operator=(LineAlternatives&&) noexcept;
public:
    // Regarding the reduction result:
    //  - If nb_alternatives == 0, the line (and therefore, the grid) is contradictory
    //  - If is_fully_reduced == true, all alternatives were computed and reduced. The value of nb_alternatives is exact
    //  - If is_fully_reduced == false, the value of nb_alternatives is an estimate (most likely, it is overestimated)
    struct Reduction
    {
        Line reduced_line = Line(Line::ROW, 0, 0);
        NbAlt nb_alternatives = 0;
        bool is_fully_reduced = false;
    };
    Reduction full_reduction();
    Reduction partial_reduction(unsigned int nb_constraints);
private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace picross

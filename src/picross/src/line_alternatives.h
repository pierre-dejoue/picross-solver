#pragma once

#include <picross/picross.h>

#include <cstddef>

namespace picross
{
class LineConstraint;

// Given a constraint, recursively build the possible alternatives of a Line and reduce them.
// If the template argument Reversed == false, the alternatives are built starting from the start of the line
// else, if Reversed == true, they are built from the end of the line
class LineAlternatives
{
public:
    LineAlternatives(const LineConstraint& constraint, const Line& known_tiles, bool reversed = false);
    ~LineAlternatives();
    // Movable
    LineAlternatives(LineAlternatives&&) noexcept;
    LineAlternatives& operator=(LineAlternatives&&) noexcept;
public:
    struct Reduction
    {
        Line reduced_line;
        unsigned int nb_alternatives;
        bool is_fully_reduced;
    };
    Reduction full_reduction();
private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;

    template <bool Reversed>
    struct BidirectionalImpl;
};

} // namespace picross

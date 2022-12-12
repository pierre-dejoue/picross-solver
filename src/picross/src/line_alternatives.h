#pragma once

#include <picross/picross.h>

namespace picross
{

// Helper class to recursively build all the possible alternatives of a Line, given a constraint
class LineAlternatives
{
public:
    LineAlternatives(const InputGrid::Constraint& segs_of_ones, const Line& known_tiles);
    ~LineAlternatives();

    unsigned int build_alternatives(unsigned int remaining_zeros);
    const Line& get_reduced_line();

private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace picross

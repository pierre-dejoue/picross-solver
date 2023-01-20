#pragma once

#include <picross/picross.h>

#include <cstddef>
#include <unordered_set>

namespace picross
{

// Hasher for OutputGrid
struct OutputGridHasher
{
    std::size_t operator()(const OutputGrid& grid) const noexcept { return grid.hash(); }
};

using OutputGridSet = std::unordered_set<OutputGrid, OutputGridHasher>;

} // namespace picross
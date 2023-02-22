#pragma once

#include <picross/picross.h>

#include <string>
#include <utility>
#include <vector>


class GridInfo
{
public:
    GridInfo(const picross::InputGrid& grid);
    GridInfo(const GridInfo&) = delete;
    GridInfo& operator=(const GridInfo&) = delete;

    void visit(bool& can_be_erased);

private:
    std::string info_as_string(unsigned int active_sections) const;

private:
    const picross::InputGrid& grid;
    std::string title;
    std::vector<std::pair<std::string, std::string>> grid_metadata;
};

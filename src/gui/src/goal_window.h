#pragma once

#include "grid_info.h"

#include <picross/picross.h>
#include <utils/grid_observer.h>

#include <string>
#include <string_view>


class Settings;

class GoalWindow
{
public:
    GoalWindow(const picross::OutputGrid& goal, std::string_view name);
    GoalWindow(const GoalWindow&) = delete;
    GoalWindow& operator=(const GoalWindow&) = delete;

    void visit(bool& can_be_erased, Settings& settings);

private:
    const ObserverGrid grid;
    const std::string title;
};

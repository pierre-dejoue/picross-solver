#include "settings.h"

#include <cassert>
#include <limits>

namespace
{

    Settings::TileLimits tile_settings_limits()
    {
        Settings::TileLimits result;

        result.size_enum.default = 2;
        result.size_enum.min = 0;
        result.size_enum.max = 4;

        result.rounding_ratio.default = 0.f;
        result.rounding_ratio.min = 0.f;
        result.rounding_ratio.max = 1.f;

        result.size_ratio.default = 1.f;
        result.size_ratio.min = 0.001f;
        result.size_ratio.max = 1.f;

        result.hide_depth_greater.default = false;
        result.hide_depth_greater.min = false;
        result.hide_depth_greater.max = true;

        result.hide_depth_value.default = 2;
        result.hide_depth_value.min = 1;
        result.hide_depth_value.max = 1000;

        return result;
    }

    Settings::SolverLimits solver_settings_limits()
    {
        Settings::SolverLimits result;

        result.limit_solutions.default = true;
        result.limit_solutions.min = false;
        result.limit_solutions.max = true;

        result.max_nb_solutions.default = 2;
        result.max_nb_solutions.min = 1;
        result.max_nb_solutions.max = std::numeric_limits<int>::max();

        return result;
    }

    Settings::AnimationLimits animation_settings_limits()
    {
        Settings::AnimationLimits result;

        result.show_branching.default = true;
        result.show_branching.min = false;
        result.show_branching.max = true;

        result.speed.default = 20;
        result.speed.min = 0;
        result.speed.max = 40;

        return result;
    }

}  // Anonymous namespace

Settings::Settings()
    : tile_settings()
    , solver_settings()
    , animation_settings()
{
}

Settings::Tile* Settings::get_tile_settings()
{
    return tile_settings.get();
}

const Settings::Tile& Settings::read_tile_settings()
{
    // Create if not existing
    if (!tile_settings)
    {
        tile_settings = std::make_unique<Tile>();
        tile_settings->size_enum = read_tile_settings_limits().size_enum.default;
        tile_settings->rounding_ratio = read_tile_settings_limits().rounding_ratio.default;
        tile_settings->size_ratio = read_tile_settings_limits().size_ratio.default;
        tile_settings->hide_depth_greater = read_tile_settings_limits().hide_depth_greater.default;
        tile_settings->hide_depth_value = read_tile_settings_limits().hide_depth_value.default;
    }
    assert(tile_settings);
    return *tile_settings;
}

const Settings::TileLimits& Settings::read_tile_settings_limits()
{
    static Settings::TileLimits result = tile_settings_limits();
    return result;
}

Settings::Solver* Settings::get_solver_settings()
{
    return solver_settings.get();
}

const Settings::Solver& Settings::read_solver_settings()
{
    // Create if not existing
    if (!solver_settings)
    {
        solver_settings = std::make_unique<Solver>();
        solver_settings->limit_solutions = read_solver_settings_limits().limit_solutions.default;
        solver_settings->max_nb_solutions = read_solver_settings_limits().max_nb_solutions.default;
    }
    assert(solver_settings);
    return *solver_settings;
}

const Settings::SolverLimits& Settings::read_solver_settings_limits()
{
    static Settings::SolverLimits result = solver_settings_limits();
    return result;
}

Settings::Animation* Settings::get_animation_settings()
{
    return animation_settings.get();
}

const Settings::Animation& Settings::read_animation_settings()
{
    // Create if not existing
    if (!animation_settings)
    {
        animation_settings = std::make_unique<Animation>();
        animation_settings->show_branching = read_animation_settings_limits().show_branching.default;
        animation_settings->speed = read_animation_settings_limits().speed.default;
    }
    assert(animation_settings);
    return *animation_settings;
}

const Settings::AnimationLimits& Settings::read_animation_settings_limits()
{
    static Settings::AnimationLimits result = animation_settings_limits();
    return result;
}

void Settings::open_window()
{
    get_settings_window();

    read_tile_settings();
    read_solver_settings();
    read_animation_settings();
}

void Settings::visit_window(bool& can_be_erased)
{
    can_be_erased = false;
    if (tile_settings || solver_settings || animation_settings || settings_window)
    {
        auto& settings_window_ref = get_settings_window();
        bool window_can_be_erased = false;
        settings_window_ref.visit(window_can_be_erased);
        assert(!window_can_be_erased); // Do not erase settings window once open
        can_be_erased &= window_can_be_erased;
    }
}

SettingsWindow& Settings::get_settings_window()
{
    if (!settings_window)
    {
        settings_window = std::make_unique<SettingsWindow>(*this);
    }
    assert(settings_window);
    return *settings_window;
}
#include "settings.h"

#include <cassert>
#include <limits>

namespace {

    Settings::UILimits ui_settings_limits()
    {
        Settings::UILimits result;

        result.extra_scaling.def = static_cast<int>(gui_style::ExtraScaling::Mid);
        result.extra_scaling.min = static_cast<int>(gui_style::ExtraScaling::Small);
        result.extra_scaling.max = static_cast<int>(gui_style::ExtraScaling::Big);

        return result;
    }

    Settings::TileLimits tile_settings_limits()
    {
        Settings::TileLimits result;

        result.size_enum.def = 2;
        result.size_enum.min = 0;
        result.size_enum.max = 4;

        result.rounding_ratio.def = 0.f;
        result.rounding_ratio.min = 0.f;
        result.rounding_ratio.max = 1.f;

        result.size_ratio.def = 1.f;
        result.size_ratio.min = 0.001f;
        result.size_ratio.max = 1.f;

        result.five_tile_border = Parameter::limits_false;

        result.hide_depth_greater = Parameter::limits_false;

        result.hide_depth_value.def = 2;
        result.hide_depth_value.min = 1;
        result.hide_depth_value.max = 1000;

        return result;
    }

    Settings::SolverLimits solver_settings_limits()
    {
        Settings::SolverLimits result;

        result.limit_solutions = Parameter::limits_true;

        result.max_nb_solutions.def = 2;
        result.max_nb_solutions.min = 1;
        result.max_nb_solutions.max = std::numeric_limits<int>::max();

        return result;
    }

    Settings::AnimationLimits animation_settings_limits()
    {
        Settings::AnimationLimits result;

        result.show_branching = Parameter::limits_true;

        result.ftl = Parameter::limits_false;

        result.one_event = Parameter::limits_false;

        result.speed.def = 20;
        result.speed.min = 0;
        result.speed.max = 40;

        return result;
    }

}  // namespace

Settings::Settings()
{
    read_ui_settings();
    read_tile_settings();
    read_solver_settings();
    read_animation_settings();
}

Settings::UI* Settings::get_ui_settings()
{
    return ui_settings.get();
}

const Settings::UI& Settings::read_ui_settings() const
{
    // Create if not existing
    if (!ui_settings)
    {
        ui_settings = std::make_unique<UI>();
        ui_settings->extra_scaling = static_cast<gui_style::ExtraScaling>(read_ui_settings_limits().extra_scaling.def);
    }
    assert(ui_settings);
    return *ui_settings;
}

const Settings::UILimits& Settings::read_ui_settings_limits()
{
    static Settings::UILimits result = ui_settings_limits();
    return result;
}

Settings::Tile* Settings::get_tile_settings()
{
    return tile_settings.get();
}

const Settings::Tile& Settings::read_tile_settings() const
{
    // Create if not existing
    if (!tile_settings)
    {
        tile_settings = std::make_unique<Tile>();
        tile_settings->size_enum = read_tile_settings_limits().size_enum.def;
        tile_settings->rounding_ratio = read_tile_settings_limits().rounding_ratio.def;
        tile_settings->size_ratio = read_tile_settings_limits().size_ratio.def;
        tile_settings->five_tile_border = read_tile_settings_limits().five_tile_border.def;
        tile_settings->hide_depth_greater = read_tile_settings_limits().hide_depth_greater.def;
        tile_settings->hide_depth_value = read_tile_settings_limits().hide_depth_value.def;
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

const Settings::Solver& Settings::read_solver_settings() const
{
    // Create if not existing
    if (!solver_settings)
    {
        solver_settings = std::make_unique<Solver>();
        solver_settings->limit_solutions = read_solver_settings_limits().limit_solutions.def;
        solver_settings->max_nb_solutions = read_solver_settings_limits().max_nb_solutions.def;
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

const Settings::Animation& Settings::read_animation_settings() const
{
    // Create if not existing
    if (!animation_settings)
    {
        animation_settings = std::make_unique<Animation>();
        animation_settings->show_branching = read_animation_settings_limits().show_branching.def;
        animation_settings->ftl = read_animation_settings_limits().ftl.def;
        animation_settings->one_event = read_animation_settings_limits().one_event.def;
        animation_settings->speed = read_animation_settings_limits().speed.def;
    }
    assert(animation_settings);
    return *animation_settings;
}

const Settings::AnimationLimits& Settings::read_animation_settings_limits()
{
    static Settings::AnimationLimits result = animation_settings_limits();
    return result;
}

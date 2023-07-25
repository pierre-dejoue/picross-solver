#pragma once

#include "parameter.h"
#include "settings_window.h"

#include <algorithm>
#include <memory>

class Settings
{
public:
    struct TileLimits
    {
        Parameter::Limits<int> size_enum;
        Parameter::Limits<float> rounding_ratio;
        Parameter::Limits<float> size_ratio;
        Parameter::Limits<bool> five_tile_border;
        Parameter::Limits<bool> hide_depth_greater;
        Parameter::Limits<int> hide_depth_value;
    };
    struct Tile
    {
        int size_enum;
        float rounding_ratio;
        float size_ratio;
        bool five_tile_border;
        bool hide_depth_greater;
        int hide_depth_value;
    };
    struct SolverLimits
    {
        Parameter::Limits<bool> limit_solutions;
        Parameter::Limits<int> max_nb_solutions;
    };
    struct Solver
    {
        bool limit_solutions;
        int max_nb_solutions;
    };
    struct AnimationLimits
    {
        Parameter::Limits<bool> show_branching;
        Parameter::Limits<bool> ftl;
        Parameter::Limits<int> speed;
    };
    struct Animation
    {
        bool show_branching;
        bool ftl;
        int speed;
    };

public:
    Settings();

    // Tile settings
    Tile* get_tile_settings();
    const Tile& read_tile_settings();
    static const TileLimits& read_tile_settings_limits();

    // Solver settings
    Solver* get_solver_settings();
    const Solver& read_solver_settings();
    static const SolverLimits& read_solver_settings_limits();

    // Animation settings
    Animation* get_animation_settings();
    const Animation& read_animation_settings();
    static const AnimationLimits& read_animation_settings_limits();

    void open_window();
    void visit_window(bool& can_be_erased);

private:
    SettingsWindow& get_settings_window();

private:
    std::unique_ptr<Tile> tile_settings;
    std::unique_ptr<Solver> solver_settings;
    std::unique_ptr<Animation> animation_settings;
    std::unique_ptr<SettingsWindow> settings_window;
};

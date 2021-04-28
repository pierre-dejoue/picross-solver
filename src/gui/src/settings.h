#pragma once

#include "settings_window.h"

#include <memory>

class Settings
{
public:
    template<typename T>
    struct Limits
    {
        T default;
        T min;
        T max;
    };
    struct TileLimits
    {
        Limits<int> size_enum;
        Limits<float> rounding_ratio;
        Limits<float> size_ratio;
        Limits<bool> show_branching;
    };
    struct Tile
    {
        int size_enum;
        float rounding_ratio;
        float size_ratio;
        bool show_branching;
    };
    struct SolverLimits
    {
        Limits<bool> limit_solutions;
        Limits<int> max_nb_solutions;
    };
    struct Solver
    {
        bool limit_solutions;
        int max_nb_solutions;
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

    void visit_windows(bool& canBeErased);

private:
    SettingsWindow& get_settings_window();

private:
    std::unique_ptr<Tile> tile_settings;
    std::unique_ptr<Solver> solver_settings;
    std::unique_ptr<SettingsWindow> settings_window;
};

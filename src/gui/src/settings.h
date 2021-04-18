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
    };
    struct Tile
    {
        int size_enum;
        float rounding_ratio;
        float size_ratio;
    };
public:
    Settings();

    // Tile settings
    Tile* get_tile_settings();
    const Tile& read_tile_settings();
    static const TileLimits& read_tile_settings_limits();

    void visit_windows(bool& canBeErased);

private:
    SettingsWindow& get_settings_window();

private:
    std::unique_ptr<Tile> tile_settings;
    std::unique_ptr<SettingsWindow> settings_window;
};

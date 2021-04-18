#include "settings.h"

#include <cassert>

namespace
{
    Settings::TileLimits tile_settings_limits()
    {
        Settings::TileLimits result;

        result.size_enum.default = 1;
        result.size_enum.min = 0;
        result.size_enum.max = 2;

        result.rounding_ratio.default = 0.f;
        result.rounding_ratio.min = 0.f;
        result.rounding_ratio.max = 1.f;

        result.size_ratio.default = 1.f;
        result.size_ratio.min = 0.001f;
        result.size_ratio.max = 1.f;

        return result;
    }
}  // Anonymous namespace

Settings::Settings()
    : tile_settings()
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
    }
    assert(tile_settings);
    return *tile_settings;
}

const Settings::TileLimits& Settings::read_tile_settings_limits()
{
    static Settings::TileLimits result = tile_settings_limits();
    return result;
}

void Settings::visit_windows(bool& canBeErased)
{
    canBeErased = false;
    if(tile_settings || settings_window)
    {
        auto& settings_window_ref = get_settings_window();
        bool windowCanBeErased = false;
        settings_window_ref.visit(windowCanBeErased);
        assert(!windowCanBeErased); // Do not erase settings window once open
        canBeErased &= windowCanBeErased;
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
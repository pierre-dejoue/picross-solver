#pragma once

#include <string>

class Settings;

class SettingsWindow
{
public:
    explicit SettingsWindow(Settings& settings);
    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    void visit(bool& can_be_erased);

private:
    struct Animation
    {
        int last_speed = 0;
        bool ftl = false;
        bool paused = false;
    };
private:
    Settings& settings;
    std::string title;
    Animation animation;
};

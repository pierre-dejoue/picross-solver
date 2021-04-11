#pragma once

#include <imgui.h>

#include <string>

class Settings;

class SettingsWindow
{
public:
    SettingsWindow(Settings& settings);
    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    void visit(bool& canBeErased);

private:
    Settings& settings;
    std::string title;

};

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
    Settings& settings;
    std::string title;

};

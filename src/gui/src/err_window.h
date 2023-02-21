#pragma once

#include <memory>
#include <string_view>

class ErrWindow
{
public:
    explicit ErrWindow(std::string_view filename);
    ~ErrWindow();
    ErrWindow(const ErrWindow&) = delete;
    ErrWindow& operator=(const ErrWindow&) = delete;

    void visit(bool& can_be_erased);
    void print(std::string_view msg);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

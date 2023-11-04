// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <ostream>
#include <string>


namespace stdutils
{
namespace platform
{

enum class OS
{
    UNKNOWN = 0,
    LINUX,
    MACOS,
    WINDOWS
};

constexpr OS os();
std::ostream& operator<<(std::ostream& out, OS os);

enum class Compiler
{
    UNKNOWN = 0,
    MSVC,
    GNU_C_CPP,
    CLANG,
    INTEL
};

constexpr Compiler compiler();
std::ostream& operator<<(std::ostream& out, Compiler compiler);
std::string compiler_version();

enum class Arch
{
    UNKNOWN = 0,
    x86,
    x86_64,
    arm64
};

constexpr Arch architecture();
std::ostream& operator<<(std::ostream& out, Arch arch);

void print_os(std::ostream& out);
void print_cpp_standard(std::ostream& out);
void print_compiler_info(std::ostream& out);
void print_compilation_date(std::ostream& out);

void print_platform_info(std::ostream& out);
void print_compiler_all_info(std::ostream& out);

}
}

#pragma once

#include <ostream>
#include <string>


namespace stdutils
{
namespace platform
{

enum class Compiler
{
    UNKNOWN = 0,
    MSVC,
    GNU_C_CPP,
    CLANG,
    INTEL
};

Compiler compiler();
std::ostream& operator<<(std::ostream& out, Compiler compiler);
std::string compiler_version();

void print_cpp_standard(std::ostream& out);
void print_compiler_info(std::ostream& out);
void print_compilation_date(std::ostream& out);

void print_compiler_all_info(std::ostream& out);

}
}
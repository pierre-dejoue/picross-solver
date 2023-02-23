#include <stdutils/platform.h>

#include <cassert>
#include <sstream>


namespace stdutils
{
namespace platform
{

Compiler compiler()
{
    // NB: Test __GNUC__ last, because that macro is sometimes defined by other compilers than the "true" GCC
    #if   defined(_MSC_VER)
        return Compiler::MSVC;
    #elif defined(__clang__)
        return Compiler::CLANG;
    #elif defined(__INTEL_COMPILER)
        return Compiler::INTEL;
    #elif defined(__GNUC__)
        return Compiler::GNU_C_CPP;
    #else
        return Compiler::UNKNOWN;
    #endif
}

std::ostream& operator<<(std::ostream& out, Compiler compiler)
{
    switch (compiler)
    {
        case Compiler::UNKNOWN:
            out << "Unknown";
            break;

        case Compiler::MSVC:
            out << "MSVC";
            break;

        case Compiler::GNU_C_CPP:
            out << "GNU GCC/G++";
            break;

        case Compiler::CLANG:
            out << "Clang";
            break;

        case Compiler::INTEL:
            out << "Intel C++";
            break;

        default:
            assert(0);
            out << "Unknown Id";
            break;
    }
    return out;
}

std::string compiler_version()
{
    std::stringstream out;
    switch (compiler())
    {
        case Compiler::UNKNOWN:
            out << "Unknown";
            break;

        case Compiler::MSVC:
            #if defined(_MSC_VER)
            out << _MSC_FULL_VER;
            #endif
            break;

        case Compiler::GNU_C_CPP:
            #if defined(__GNUC__)
            out << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
            #endif
            break;

        case Compiler::CLANG:
            #if defined(__clang__)
            out << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
            #endif
            break;

        case Compiler::INTEL:
            #if defined(__INTEL_COMPILER)
            out << __INTEL_COMPILER;
            #endif
            break;

        default:
            assert(0);
            out << "Unknown Id";
            break;
    }
    return out.str();
}

void print_cpp_standard(std::ostream& out)
{
    out << "C++ Standard: " << __cplusplus << std::endl;
}

void print_compiler_info(std::ostream& out)
{
    const auto compiler_id = compiler();
    out << "Compiler: " << compiler_id;
    if (compiler_id != Compiler::UNKNOWN)
        out << " " << compiler_version();
    out << std::endl;
}

void print_compilation_date(std::ostream& out)
{
    out << "Compilation date: " << __DATE__ << " " << __TIME__ << std::endl;
}

void print_compiler_all_info(std::ostream& out)
{
    print_cpp_standard(out);
    print_compiler_info(out);
    print_compilation_date(out);
}


} // namespace platform
} // namespace stdutils
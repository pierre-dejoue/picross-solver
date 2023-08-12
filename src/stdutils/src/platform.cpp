#include <stdutils/platform.h>

#include <cassert>
#include <sstream>


namespace stdutils
{
namespace platform
{

constexpr Compiler compiler()
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

constexpr Arch architecture()
{
    // Source: https://abseil.io/docs/cpp/platforms/macros
    #if   defined(__aarch64__)
        return Arch::arm64;
    #elif defined(__x86_64__) || defined(_M_X64)
        return Arch::x86_64;
    #elif defined(__i386__) || defined(_M_IX32)
        return Arch::x86;
    #else
        return Arch::UNKNOWN;
    #endif
}

std::ostream& operator<<(std::ostream& out, Arch arch)
{
    switch (arch)
    {
        case Arch::UNKNOWN:
            out << "Unknown";
            break;

        case Arch::x86:
            out << "x86";
            break;

        case Arch::x86_64:
            out << "x86_64";
            break;

        case Arch::arm64:
            out << "arm64";
            break;

        default:
            assert(0);
            out << "Unknown Id";
            break;
    }
    return out;
}

void print_cpp_standard(std::ostream& out)
{
    out << "C++ Standard: " << __cplusplus << std::endl;
}

void print_compiler_info(std::ostream& out)
{
    constexpr auto compiler_id = compiler();
    out << "Compiler: " << compiler_id;
    if constexpr (compiler_id != Compiler::UNKNOWN)
        out << " " << compiler_version();
    out << std::endl;
}

void print_architecture_info(std::ostream& out)
{
    constexpr auto arch_id = architecture();
    out<< "Arch: " << arch_id << std::endl;
}

void print_compilation_date(std::ostream& out)
{
    out << "Compilation date: " << __DATE__ << " " << __TIME__ << std::endl;
}

void print_compiler_all_info(std::ostream& out)
{
    print_cpp_standard(out);
    print_compiler_info(out);
    print_architecture_info(out);
    print_compilation_date(out);
}

} // namespace platform
} // namespace stdutils
#include <ostream>

void print_compiler_info(std::ostream& out)
{
    out << "C++ Standard: " << __cplusplus << std::endl;
    #if   defined(_MSC_VER)
        out << "Compiler: MSVC " << _MSC_FULL_VER << std::endl;
    #elif defined(__GNUC__)
        out << "Compiler: GNU GCC/G++ " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << std::endl;
    #elif defined(__clang__)
        out << "Compiler: Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__ << std::endl;
    #elif defined(__INTEL_COMPILER)
        out << "Compiler: Intel C++ Compiler " << __INTEL_COMPILER << std::endl;
    #else
        out << "Unknown compiler" << std::endl;
    #endif
    out << "Compilation date: " << __DATE__ << " " << __TIME__ << std::endl;
}

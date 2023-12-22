// Copyright (c) 2021 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#include <stdutils/string.h>

#include <algorithm>
#include <cctype>


namespace stdutils
{
namespace string
{

std::string tolower(const std::string& in)
{
    std::string out(in);
    // Conversion from char to unsigned char is important. See STL doc.
    std::transform(in.cbegin(), in.cend(), out.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string toupper(const std::string& in)
{
    std::string out(in);
    // Conversion from char to unsigned char is important. See STL doc.
    std::transform(in.cbegin(), in.cend(), out.begin(), [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

std::string capitalize(const std::string& in)
{
    std::string out = tolower(in);
    if (!out.empty()) {  out.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(out.front()))); }
    return out;
}

} // namespace string
} // namespace stdutils

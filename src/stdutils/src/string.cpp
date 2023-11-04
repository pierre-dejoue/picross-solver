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
    std::transform(in.cbegin(), in.cend(), out.begin(), [](const char& c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string capitalize(const std::string& in)
{
    std::string out = tolower(in);
    if (!out.empty())
        out.front() = static_cast<char>(std::toupper(out.front()));
    return out;
}

std::string file_extension(std::string_view filepath)
{
 std::string result(filepath);
    const auto pos = filepath.find_last_of('.');
    if (pos != std::string::npos)
        return result.substr(pos + 1);
    else
        return "";
}

std::string filename(std::string_view filepath)
{
    std::string_view result(filepath);
    const auto pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
        result = filepath.substr(pos + 1);
    return std::string(result);
}

std::string filename_wo_extension(std::string_view filepath)
{
    const std::string name = filename(filepath);
    const auto pos = name.find_last_of('.');
    if (pos != std::string::npos)
        return name.substr(0, pos);
    else
        return name;
}

} // namespace string
} // namespace stdutils

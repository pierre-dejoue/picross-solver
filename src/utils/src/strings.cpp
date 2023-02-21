#include <utils/strings.h>

#include <algorithm>
#include <cctype>

std::string str_tolower(const std::string& in)
{
    std::string out(in);
    std::transform(in.cbegin(), in.cend(), out.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
    return out;
}

std::string str_capitalize(const std::string& in)
{
    std::string out = str_tolower(in);
    if (!out.empty())
        out.front() = std::toupper(out.front());
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

std::string file_name(std::string_view filepath)
{
    std::string_view result(filepath);
    const auto pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
        result = filepath.substr(pos + 1);
    return std::string(result);
}

std::string file_name_wo_extension(std::string_view filepath)
{
    const std::string filename = file_name(filepath);
    const auto pos = filename.find_last_of('.');
    if (pos != std::string::npos)
        return filename.substr(0, pos);
    else
        return filename;
}

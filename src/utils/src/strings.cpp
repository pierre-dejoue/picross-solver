#include <utils/strings.h>

#include <algorithm>
#include <cctype>

std::string str_tolower(const std::string& in)
{
    std::string out(in);
    std::transform(in.cbegin(), in.cend(), out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

std::string file_extension(const std::string& filepath)
{
    const auto pos = filepath.find_last_of('.');
    if (pos != std::string::npos)
        return filepath.substr(pos + 1);
    else
        return "";
}

std::string file_name(const std::string& filepath)
{
    const auto pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
        return filepath.substr(pos + 1);
    else
        return filepath;
}

std::string file_name_wo_extension(const std::string& filepath)
{
    const std::string filename = file_name(filepath);
    const auto pos = filename.find_last_of('.');
    if (pos != std::string::npos)
        return filename.substr(0, pos);
    else
        return filename;
}

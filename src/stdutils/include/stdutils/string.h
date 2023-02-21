#pragma once

#include <string>
#include <string_view>


namespace stdutils
{
namespace string
{

std::string tolower(const std::string& in);

std::string capitalize(const std::string& in);

std::string file_extension(std::string_view filepath);

std::string filename(std::string_view filepath);

std::string filename_wo_extension(std::string_view filepath);

} // namespace string
} // namespace stdutils

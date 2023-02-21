#pragma once

#include <string>
#include <string_view>

std::string str_tolower(const std::string& in);

std::string str_capitalize(const std::string& in);

std::string file_extension(std::string_view filepath);

std::string file_name(std::string_view filepath);

std::string file_name_wo_extension(std::string_view filepath);

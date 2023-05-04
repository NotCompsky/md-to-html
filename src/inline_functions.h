#pragma once

#include <string>
#include <string_view>
#include <vector>

std::string concat_string_views(std::string_view s1, std::string_view s2);
std::string parse_inline_function_command(const std::string& command);

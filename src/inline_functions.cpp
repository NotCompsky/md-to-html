#include "inline_functions.h"

#include <string>
#include <string_view>

std::string concat_string_views(std::string_view s1, std::string_view s2)
{
std::string result;
result.reserve(s1.size() + s2.size());
result += s1;
result += s2;
return result;
}

std::string parse_inline_function_command(const std::string& command)
{
// extract function name and arguments
size_t pos = command.find('(');
std::string function_name = command.substr(0, pos);
std::string args_str = command.substr(pos + 1, command.size() - pos - 2); // exclude parentheses

// parse function arguments
std::vector<std::string> args;
size_t arg_start = 0;
bool in_string = false;
for (size_t i = 0; i < args_str.size(); ++i)
{
char c = args_str[i];
if (c == '"')
{
in_string = !in_string;
}
else if (c == ',' && !in_string)
{
args.push_back(args_str.substr(arg_start, i - arg_start));
arg_start = i + 1;
}
}
args.push_back(args_str.substr(arg_start)); // add final argument

// call function based on name
if (function_name == "concat_string_views")
{
if (args.size() != 2)
{
return "Error: concat_string_views expects 2 arguments";
}
return concat_string_views(args[0], args[1]);
}
else
{
return "Error: unknown function " + function_name;
}
}

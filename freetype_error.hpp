#pragma once

#include <string>

using FT_Error = int;

[[nodiscard]] std::string ft_error_string(const FT_Error error_code);

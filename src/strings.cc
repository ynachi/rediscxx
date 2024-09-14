//
// Created by ynachi on 9/14/24.
//
#include <algorithm>
#include <cctype>
#include <string>
#include <strings.h>

namespace utils
{
    std::string to_upper(const std::string& str) noexcept
    {
        std::string result = str;
        std::ranges::transform(result, result.begin(), ::toupper);
        return result;
    }
}  // namespace utils

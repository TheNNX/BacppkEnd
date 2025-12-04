#pragma once

#include <string>

inline std::vector<std::string> SplitString(const std::string& s, char separator = ' ')
{
    std::vector<std::string> parts;
    auto currentIt = s.begin();
    auto prev = s.begin();

    while (currentIt != s.end())
    {
        currentIt = std::find(currentIt + 1, s.end(), separator);
        parts.push_back(std::string(prev, currentIt));
        if (currentIt != s.end())
        {
            prev = currentIt + 1;
        }
    }

    return parts;
}

inline std::string Trim(std::string value)
{
    while (value.starts_with(' '))
    {
        value = value.substr(1);
    }
    while (value.ends_with(' '))
    {
        value = value.substr(0, value.length() - 1);
    }

    return value;
}
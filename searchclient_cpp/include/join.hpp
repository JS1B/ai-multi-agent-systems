#ifndef JOIN_HPP
#define JOIN_HPP

#include <string>
#include <vector>

namespace utils
{
    std::string join(const std::vector<std::string> &strings, const std::string &separator)
    {
        std::string result;
        for (size_t i = 0; i < strings.size(); i++)
        {
            result += strings[i];
            if (i < strings.size() - 1)
            {
                result += separator;
            }
        }
        return result;
    }
}

#endif
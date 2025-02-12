#include "color.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

// Define the colorNamesMap
std::map<std::string, Color> colorNamesMap = {
    {"none", Color::None},
    {"red", Color::Red},
    {"green", Color::Green},
    {"blue", Color::Blue},
    {"orange", Color::Orange},
    {"purple", Color::Purple},
    {"pink", Color::Pink},
    {"brown", Color::Brown},
    {"gray", Color::Gray},
    {"yellow", Color::Yellow}};

Color colorFromString(const std::string &colorStr)
{
    std::string lowerColorStr = colorStr;
    std::transform(lowerColorStr.begin(), lowerColorStr.end(), lowerColorStr.begin(), ::tolower);
    if (colorNamesMap.count(lowerColorStr))
    {
        return colorNamesMap[lowerColorStr];
    }
    else
    {
        std::cerr << "Error: invalid color " << colorStr << std::endl;
        return Color::None;
    }
}
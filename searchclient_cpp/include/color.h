#ifndef SEARCHCLIENT_COLOR_H
#define SEARCHCLIENT_COLOR_H

#include <string>
#include <map>

enum class Color
{
    None,
    Red,
    Green,
    Blue,
    Orange,
    Purple,
    Pink,
    Brown,
    Gray,
    Yellow
};

Color colorFromString(const std::string &color);

extern std::map<std::string, Color> colorNamesMap;

#endif // SEARCHCLIENT_COLOR_H
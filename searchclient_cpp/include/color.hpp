#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>

enum class Color {
    Blue,
    Red,
    Cyan,
    Purple,
    Green,
    Orange,
    Pink,
    Grey,
    Lightblue,
    Brown
};

inline Color from_string(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "blue") return Color::Blue;
    if (lower == "red") return Color::Red;
    if (lower == "cyan") return Color::Cyan;
    if (lower == "purple") return Color::Purple;
    if (lower == "green") return Color::Green;
    if (lower == "orange") return Color::Orange;
    if (lower == "pink") return Color::Pink;
    if (lower == "grey") return Color::Grey;
    if (lower == "lightblue") return Color::Lightblue;
    if (lower == "brown") return Color::Brown;

    throw std::invalid_argument("Invalid color string: " + s);
}
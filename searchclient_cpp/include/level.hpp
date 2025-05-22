#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "color.hpp"
#include "chargrid.hpp"
#include "cell2d.hpp"

// constexpr would be better (modern C++)
#define WALL '+'
#define EMPTY ' '
#define FIRST_AGENT '0'
#define LAST_AGENT '9'
#define FIRST_BOX 'A'
#define LAST_BOX 'Z'
#define FIRST_GOAL '0'
#define LAST_GOAL '9'

class Level {
   public:
    Level() = delete;
    Level(const std::vector<Cell2D> &agents, const CharGrid &boxes);
    Level(const Level &) = default;
    Level &operator=(const Level &) = default;
    ~Level() = default;

    bool operator==(const Level &other) const;

    static std::string name;
    static std::string domain;

    static CharGrid walls;
    static CharGrid box_goals;
    static std::vector<Cell2D> agent_goals;

    static std::vector<Color> agent_colors;
    static std::vector<Color> box_colors;
    
    std::vector<Cell2D> agents;
    CharGrid boxes;

    std::string toString();
};

Level loadLevel(std::istream &serverMessages);

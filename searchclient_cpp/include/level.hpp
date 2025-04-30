#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "color.hpp"

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
    Level(std::istream &istream = std::cin);
    Level(const Level &) = default;
    Level &operator=(const Level &) = default;
    ~Level() = default;

    std::vector<std::vector<bool>> walls;

    std::vector<int> agentRows;
    std::vector<int> agentCols;
    std::vector<Color> agentColors;

    std::vector<std::vector<char>> boxes;
    std::vector<Color> boxColors;

    std::vector<std::vector<char>> goals;

    std::string toString();

   private:
    std::string domain_;
    std::string name_;
    void loadLevel(std::istream &serverMessages);
};

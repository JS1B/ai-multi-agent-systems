#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "agent.hpp"
#include "box.hpp"
#include "color.hpp"
#include "goal.hpp"

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
    Level(const std::unordered_map<char, Agent> &agentsMap, const std::unordered_map<char, Box> &boxesMap);
    Level(const Level &) = default;
    Level &operator=(const Level &) = default;
    ~Level() = default;

    static std::vector<std::vector<bool>> walls;
    static std::unordered_map<char, Goal> goalsMap;

    std::unordered_map<char, Agent> agentsMap;
    std::unordered_map<char, Box> boxesMap;

    std::string toString();

    static std::string domain;
    static std::string name;
};

Level loadLevel(std::istream &serverMessages);

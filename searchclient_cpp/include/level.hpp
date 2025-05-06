#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "action.hpp"
#include "agent.hpp"
#include "box.hpp"
#include "goal.hpp"
#include "point2d.hpp"

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
    Level(const std::unordered_map<char, Agent> &agentsMap, const std::unordered_map<char, Box> &boxesMap,
          const std::vector<std::vector<char>> &grid_layout);
    Level(const Level &) = default;
    Level &operator=(const Level &) = default;
    ~Level() = default;

    std::string toString();

    static std::string domain;
    static std::string name;
    static std::vector<std::vector<bool>> walls;
    static std::unordered_map<char, Goal> goalsMap;

    std::unordered_map<char, Agent> agentsMap;
    std::unordered_map<char, Box> boxesMap;

    bool isCellEmpty(const Point2D &position) const;
    bool isCellBox(const Point2D &position) const;
    bool isCellAgent(const Point2D &position) const;
    char charAt(const Point2D &position) const;

    void moveAgent(const char id, const Action *&action);
    void moveBox(const char id, const Action *&action);

    bool operator==(const Level &other) const;
    bool operator!=(const Level &other) const;

   private:
    // @todo: test
    std::vector<std::vector<char>> grid_layout_;
};

Level loadLevel(std::istream &serverMessages);

#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>

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
    Level(const std::unordered_map<char, Agent> &agentsMap, const std::unordered_map<char, Box> &boxesMap,
          std::vector<std::vector<bool>> walls,
          std::unordered_map<char, Goal> goalsMap, 
          std::map<char, Point2D> goalsMap_for_Point2D,
          std::string domain, std::string name, int rows, int cols);
    Level(const Level &) = default;
    Level &operator=(const Level &) = delete;
    ~Level() = default;

    std::vector<std::vector<bool>> walls_;
    std::unordered_map<char, Goal> goalsMap_;
    std::map<char, Point2D> goalsMap_for_Point2D_;
    std::string domain_;
    std::string name_;
    int rows_ = 0;
    int cols_ = 0;

    const std::unordered_map<char, Agent> agentsMap;
    const std::unordered_map<char, Box> boxesMap;

    // Pre-sorted IDs for consistent iteration order in hashing/equality
    std::vector<char> sortedAgentIds_;
    std::vector<char> sortedBoxIds_;

    std::string toString() const;

    bool isWall(int r_y, int c_x) const;
    char getEntityAt(int r, int c) const;
    Point2D getGoalPosition(char entity_id) const;

    int getRows() const { return rows_; }
    int getCols() const { return cols_; }

    // Helper to get all agent characters (IDs)
    std::vector<char> getAgentChars() const;
    std::vector<char> getBoxChars() const;

    // Getter methods
    const std::string& getDomain() const { return domain_; }
    const std::string& getName() const { return name_; }
    const std::vector<std::vector<bool>>& getWalls() const { return walls_; }
    const std::unordered_map<char, Goal>& getGoalsMap() const { return goalsMap_; }
    const std::map<char, Point2D>& getGoalsMapForPoint2D() const { return goalsMap_for_Point2D_; }
    const std::vector<char>& getSortedAgentIds() const { return sortedAgentIds_; }
    const std::vector<char>& getSortedBoxIds() const { return sortedBoxIds_; }
};

Level loadLevel(std::istream &serverMessages);

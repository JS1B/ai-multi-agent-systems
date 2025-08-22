#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "agent.hpp"
#include "box_bulk.hpp"
#include "cell2d.hpp"
#include "chargrid.hpp"
#include "color.hpp"

#define WALL '+'
#define EMPTY ' '
#define FIRST_AGENT '0'
#define LAST_AGENT '9'
#define FIRST_BOX 'A'
#define LAST_BOX 'Z'

class StaticLevel {
   private:
    std::string name_;
    std::string domain_;
    CharGrid walls_;

    std::map<char, Color> agent_colors_;
    std::map<char, Color> box_colors_;

   public:
    StaticLevel() = delete;
    StaticLevel(std::string name, std::string domain, CharGrid walls, std::map<char, Color> agent_colors, std::map<char, Color> box_colors);
    ~StaticLevel() = default;

    bool isCellFree(const Cell2D &cell) const;
    inline const std::string getDomain() const { return domain_; }
    inline const std::string getName() const { return name_; }
    inline const Cell2D getSize() const { return walls_.size(); }

    std::string toString() const;
};

class Level {
   public:
    StaticLevel static_level;
    std::vector<Agent> agents;
    // std::vector<BoxBulk> boxes;

    Level(StaticLevel static_level, std::vector<Agent> agents);
    ~Level() = default;

    std::string toString() const;
};

Level loadLevel(std::istream &serverMessages);

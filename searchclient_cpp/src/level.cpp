#include "level.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "helpers.hpp"

std::vector<std::vector<bool>> Level::walls;
std::unordered_map<char, Goal> Level::goalsMap;
std::string Level::domain;
std::string Level::name;

Level::Level(const std::unordered_map<char, Agent> &agentsMap, const std::unordered_map<char, Box> &boxesMap,
             const std::vector<std::vector<char>> &grid_layout)
    : agentsMap(agentsMap), boxesMap(boxesMap), grid_layout_(grid_layout) {}

std::string Level::toString() {
    std::stringstream ss;
    ss << "Level(" << domain << ", " << name << ", " << walls.size() << "x" << walls[0].size() << ")";
    return ss.str();
}
bool Level::isCellEmpty(const Point2D &position) const { return charAt(position) == EMPTY; }

bool Level::isCellBox(const Point2D &position) const {
    const char c = charAt(position);
    return c >= FIRST_BOX && c <= LAST_BOX;
}

bool Level::isCellAgent(const Point2D &position) const {
    const char c = charAt(position);
    return c >= FIRST_AGENT && c <= LAST_AGENT;
}

char Level::charAt(const Point2D &position) const { return grid_layout_[position.x()][position.y()]; }

void Level::moveAgent(const char id, const Action *&action) {
    Agent &agent = agentsMap.at(id);
    grid_layout_[agent.position().x()][agent.position().y()] = EMPTY;
    agent.moveBy(action->agentDelta);
    grid_layout_[agent.position().x()][agent.position().y()] = id;
}

void Level::moveBox(const char id, const Action *&action) {
    Box &box = boxesMap.at(id);
    grid_layout_[box.position().x()][box.position().y()] = EMPTY;
    box.moveBy(action->boxDelta);
    grid_layout_[box.position().x()][box.position().y()] = id;
}

Level loadLevel(std::istream &serverMessages) {
    // Read domain (skip)
    std::string line;
    getline(serverMessages, line);  // #domain
    assert(line == "#domain");
    getline(serverMessages, line);  // hospital
    Level::domain = line;

    // Read level name (skip)
    getline(serverMessages, line);  // #levelname
    assert(line == "#levelname");
    getline(serverMessages, line);  // <name>
    Level::name = line;

    // Read colors
    getline(serverMessages, line);  // #colors
    assert(line == "#colors");
    getline(serverMessages, line);

    std::vector<std::string> colorSection;
    colorSection.reserve(10);
    while (line.find("#") == std::string::npos) {
        colorSection.push_back(utils::trim(line));
        getline(serverMessages, line);
    }

    std::unordered_map<char, Color> agentColors;
    std::unordered_map<char, Color> boxColors;

    for (const std::string &line : colorSection) {
        std::string colorStr, entitiesStr;
        std::stringstream ss(line);

        getline(ss, colorStr, ':');
        getline(ss, entitiesStr);

        const Color color = from_string(colorStr);

        // Reads agents, colors with ids and box, colors with ids with unnormilized whitespaces
        std::stringstream entitiesStream(entitiesStr);
        std::string entity;
        while (getline(entitiesStream, entity, ',')) {
            entity = utils::normalizeWhitespace(entity);
            entity = utils::trim(entity);
            if (entity.length() != 1) {
                continue;
            }

            const char entityChar = entity[0];
            if (FIRST_AGENT <= entityChar && entityChar <= LAST_AGENT) {
                agentColors.insert({entityChar, color});
                continue;
            }

            if (FIRST_BOX <= entityChar && entityChar <= LAST_BOX) {
                boxColors.insert({entityChar, color});
                continue;
            }
        }
    }

    // Read initial state
    std::vector<std::string> levelLines;
    levelLines.reserve(100);

    getline(serverMessages, line);
    while (line.find("#") == std::string::npos) {
        levelLines.push_back(line);
        getline(serverMessages, line);
    }

    const int numRows = levelLines.size();
    const int numCols = std::max_element(levelLines.begin(), levelLines.end(), [](const std::string &a, const std::string &b) {
                            return a.length() < b.length();
                        })->length();

    std::vector<std::vector<char>> grid_layout;
    grid_layout.resize(numRows, std::vector<char>(numCols, EMPTY));

    std::vector<std::vector<bool>> walls(numRows, std::vector<bool>(numCols, false));

    std::unordered_map<char, Agent> agentsMap;
    std::unordered_map<char, Box> boxesMap;

    for (int row = 0; row < numRows; row++) {
        const std::string &line = levelLines[row];
        for (int col = 0; col < (int)line.length(); col++) {
            const char c = line[col];
            grid_layout[row][col] = c;
            if (FIRST_AGENT <= c && c <= LAST_AGENT) {
                agentsMap.insert({c, Agent(c, row, col, agentColors[c])});
                continue;
            }
            if (FIRST_BOX <= c && c <= LAST_BOX) {
                boxesMap.insert({c, Box(c, row, col, boxColors[c])});
                continue;
            }
            if (c == WALL) {
                walls[row][col] = true;
                continue;
            }
        }
    }

    walls.shrink_to_fit();
    for (auto &row : walls) {
        row.shrink_to_fit();
    }
    Level::walls = walls;

    grid_layout.shrink_to_fit();
    for (auto &row : grid_layout) {
        row.shrink_to_fit();
    }
    // Read goal state
    std::unordered_map<char, Goal> goalsMap;
    std::vector<std::string> goalLines;
    getline(serverMessages, line);  // first line of goal state
    while (line.find("#") == std::string::npos) {
        goalLines.push_back(line);
        getline(serverMessages, line);
    }

    for (int row = 0; row < numRows; row++) {
        const std::string &line = goalLines[row];
        for (int col = 0; col < (int)line.length(); col++) {
            const char c = line[col];
            if ((FIRST_BOX <= c && c <= LAST_BOX) || (FIRST_AGENT <= c && c <= LAST_AGENT)) {
                goalsMap.insert({c, Goal(c, row, col)});
            }
        }
    }

    Level::goalsMap = goalsMap;
    return Level(agentsMap, boxesMap, grid_layout);
}

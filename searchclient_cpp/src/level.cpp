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

Level::Level(const std::unordered_map<char, Agent> &agentsMap, const std::unordered_map<char, Box> &boxesMap)
    : agentsMap(agentsMap), boxesMap(boxesMap) {}

std::string Level::toString() {
    std::stringstream ss;
    ss << "Level(" << domain << ", " << name << ", " << walls.size() << "x" << walls[0].size() << ")";
    return ss.str();
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
    std::vector<std::vector<bool>> walls(numRows, std::vector<bool>(numCols, false));

    std::unordered_map<char, Agent> agentsMap;
    std::unordered_map<char, Box> boxesMap;

    for (int row = 0; row < numRows; row++) {
        const std::string &line = levelLines[row];
        for (int col = 0; col < (int)line.length(); col++) {
            const char c = line[col];
            if (FIRST_AGENT <= c && c <= LAST_AGENT) {
                agentsMap.insert({c, Agent(c, row, col, agentColors[c])});
            } else if (FIRST_BOX <= c && c <= LAST_BOX) {
                boxesMap.insert({c, Box(c, row, col, boxColors[c])});
            } else if (c == WALL) {
                walls[row][col] = true;
            }
        }
    }

    walls.shrink_to_fit();
    for (auto &row : walls) {
        row.shrink_to_fit();
    }

    Level::walls = walls;

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
            char c = line[col];
            if ((FIRST_BOX <= c && c <= LAST_BOX) || (FIRST_AGENT <= c && c <= LAST_AGENT)) {
                goalsMap.insert({c, Goal(c, row, col)});
            }
        }
    }

    Level::goalsMap = goalsMap;
    return Level(agentsMap, boxesMap);
}

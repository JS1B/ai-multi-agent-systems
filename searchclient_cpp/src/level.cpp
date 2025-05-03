#include "level.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "string_manip_helpers.hpp"

std::vector<std::vector<bool>> Level::walls;
std::unordered_map<char, Goal> Level::goalsMap;

Level::Level(std::istream &istream) { loadLevel(istream); }

std::string Level::toString() {
    std::stringstream ss;
    ss << "Level(" << domain_ << ", " << name_ << ", " << walls.size() << "x" << walls[0].size() << ")";
    return ss.str();
}

void Level::loadLevel(std::istream &serverMessages) {
    // Read domain (skip)
    std::string line;
    getline(serverMessages, line);  // #domain
    assert(line == "#domain");
    getline(serverMessages, line);  // hospital
    domain_ = line;

    // Read level name (skip)
    getline(serverMessages, line);  // #levelname
    assert(line == "#levelname");
    getline(serverMessages, line);  // <name>
    name_ = line;

    // Read colors
    getline(serverMessages, line);  // #colors
    assert(line == "#colors");
    getline(serverMessages, line);

    std::vector<std::string> colorSection;
    // colorSection.reserve(10);
    while (line.find("#") == std::string::npos) {
        colorSection.push_back(utils::trim(line));
        getline(serverMessages, line);
    }

    std::vector<Color> agentColors;
    std::vector<Color> boxColors;
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
            if (entity.length() != 1) {
                continue;
            }

            char entityChar = entity[0];
            if (FIRST_AGENT <= entityChar && entityChar <= LAST_AGENT) {
                agentColors.push_back(color);
                continue;
            }

            if (FIRST_BOX <= entityChar && entityChar <= LAST_BOX) {
                boxColors.push_back(color);
                continue;
            }
        }
    }

    // Read initial state
    std::vector<std::string> levelLines;

    getline(serverMessages, line);
    while (line.find("#") == std::string::npos) {
        levelLines.push_back(line);
        getline(serverMessages, line);
    }

    const int numRows = levelLines.size();
    const int numCols = levelLines[0].length();
    std::vector<std::vector<bool>> walls(numRows, std::vector<bool>(numCols, false));

    for (int row = 0; row < numRows; row++) {
        for (int col = 0; col < numCols; col++) {
            char c = levelLines[row][col];
            if (FIRST_AGENT <= c && c <= LAST_AGENT) {
                agentsMap.insert({c, Agent(c, row, col, agentColors[c - FIRST_AGENT])});
            } else if (FIRST_BOX <= c && c <= LAST_BOX) {
                boxesMap.insert({c, Box(c, row, col, boxColors[c - FIRST_BOX])});
            } else if (c == WALL) {
                walls[row][col] = true;
            }
        }
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
        for (int col = 0; col < numCols; col++) {
            char c = goalLines[row][col];
            if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z')) {
                goalsMap.insert({c, Goal(c, row, col)});
            }
        }
    }
    Level::goalsMap = goalsMap;
    // return State(agentRows, agentCols, agentColors, walls, boxes, boxColors, goals);
}

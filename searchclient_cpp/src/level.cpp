#include "level.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
    getline(serverMessages, line);  // hospital
    domain_ = line;

    // Read level name (skip)
    getline(serverMessages, line);  // #levelname
    getline(serverMessages, line);  // <name>
    name_ = line;

    // Read colors
    getline(serverMessages, line);  // #colors

    getline(serverMessages, line);
    while (line.find("#") == std::string::npos) {
        std::stringstream ss(line);
        std::string colorStr, entitiesStr;
        getline(ss, colorStr, ':');
        getline(ss, entitiesStr);

        const Color color = from_string(colorStr);

        // Reads agents, colors with ids and box, colors with ids with unnormilized whitespaces
        std::stringstream entitiesStream(entitiesStr);
        std::string entity;
        while (getline(entitiesStream, entity, ',')) {
            entity.erase(std::remove_if(entity.begin(), entity.end(), (int (*)(int))std::isspace), entity.end());  // Trim whitespace
            if (entity.length() == 1) {
                char c = entity[0];
                if ('0' <= c && c <= '9') {
                    agentColors.push_back(color);
                } else if ('A' <= c && c <= 'Z') {
                    boxColors.push_back(color);
                }
            }
        }
        getline(serverMessages, line);  // next agent color or #initial
    }

    // Read initial state
    int numRows = 0;
    int numCols = 0;
    std::vector<std::string> levelLines;

    getline(serverMessages, line);
    while (line.find("#") == std::string::npos) {
        levelLines.push_back(line);
        numCols = std::max(numCols, (int)line.length());
        numRows++;
        getline(serverMessages, line);
    }

    walls.resize(numRows, std::vector<bool>(numCols, false));
    boxes.resize(numRows, std::vector<char>(numCols, EMPTY));

    for (int row = 0; row < numRows; ++row) {
        for (size_t col = 0; col < levelLines[row].length(); ++col) {
            char c = levelLines[row][col];
            if ('0' <= c && c <= '9') {
                agentRows.push_back(row);
                agentCols.push_back(col);
            } else if ('A' <= c && c <= 'Z') {
                boxes[row][col] = c;
            } else if (c == '+') {
                walls[row][col] = true;
            }
        }
    }

    // Read goal state
    goals.resize(numRows, std::vector<char>(numCols, EMPTY));
    getline(serverMessages, line);  // first line of goal state
    int row = 0;
    while (line.find("#") == std::string::npos) {
        for (size_t col = 0; col < line.length(); ++col) {
            char c = line[col];
            if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z')) {
                goals[row][col] = c;
            }
        }
        row++;
        getline(serverMessages, line);
    }
    // return State(agentRows, agentCols, agentColors, walls, boxes, boxColors, goals);
}

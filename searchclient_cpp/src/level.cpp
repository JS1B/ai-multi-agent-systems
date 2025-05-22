#include "level.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "helpers.hpp"

std::string Level::name;
std::string Level::domain;
CharGrid Level::walls(0, 0);
CharGrid Level::box_goals(0, 0);
std::vector<Cell2D> Level::agent_goals(10, Cell2D(0, 0));
std::vector<Color> Level::agent_colors(10);
std::vector<Color> Level::box_colors(26);

//Level::Level(const std::unordered_map<char, Agent> &agentsMap, const std::unordered_map<char, Box> &boxesMap)
//    : agentsMap(agentsMap), boxesMap(boxesMap) {}

Level::Level(const std::vector<Cell2D> &agents, const CharGrid &boxes)
    : agents(agents), boxes(boxes) {}

std::string Level::toString() {
    std::stringstream ss;
    ss << "Level(" << domain << ", " << name << ", " << walls.size_rows() << "x" << walls.size_cols() << ")";
    return ss.str();
}

Level loadLevel(std::istream &serverMessages) {
    // Reset static members
    Level::domain = "";
    Level::name = "";
    Level::walls = CharGrid(0, 0);
    Level::box_goals = CharGrid(0, 0);
    Level::agent_goals = std::vector<Cell2D>(10, Cell2D(0, 0));
    Level::agent_colors = std::vector<Color>(10);
    Level::box_colors = std::vector<Color>(26);
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
                Level::agent_colors[entityChar - '0'] = color;
                continue;
            }

            if (FIRST_BOX <= entityChar && entityChar <= LAST_BOX) {
                Level::box_colors[entityChar - 'A'] = color;
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

    Level::walls = CharGrid(numRows, numCols);
    Level::box_goals = CharGrid(numRows, numCols);
    std::vector<Cell2D> agents(10);
    CharGrid boxes(numRows, numCols);

    int num_agents = 0;
    for (int row = 0; row < numRows; row++) {
        const std::string &line = levelLines[row];
        for (int col = 0; col < (int)line.length(); col++) {
            const char c = line[col];
            if (FIRST_AGENT <= c && c <= LAST_AGENT) {
                agents[c - '0'] = Cell2D(row, col);
                num_agents++;
            } else if (FIRST_BOX <= c && c <= LAST_BOX) {
                boxes(row, col) = c;
            } else if (c == WALL) {
                Level::walls(row, col) = c;
            }
        }
    }

    agents.resize(num_agents); // from now on number of agents is stored as agents.size()

    // Read goal state
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
            if ((FIRST_BOX <= c && c <= LAST_BOX)) {
                Level::box_goals(row, col) = c;
            } else if (FIRST_AGENT <= c && c <= LAST_AGENT) {
                Level::agent_goals[c - '0'] = Cell2D(row, col);
            }
        }
    }

    return Level(agents, boxes);
}

bool Level::operator==(const Level &other) const {
    return agents == other.agents && boxes == other.boxes;
}

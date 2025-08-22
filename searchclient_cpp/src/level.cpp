#include "level.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "utils.hpp"

StaticLevel::StaticLevel(std::string name, std::string domain, CharGrid walls, std::map<char, Color> agent_colors,
                         std::map<char, Color> box_colors)
    : name_(name), domain_(domain), walls_(walls), agent_colors_(agent_colors), box_colors_(box_colors) {}

bool StaticLevel::isCellFree(const Cell2D &cell) const {
    // if (cell.r < 0 || cell.r >= walls.size_rows() || cell.c < 0 || cell.c >= walls.size_cols()) {
    //     return false;
    // }
    if (walls_(cell) == WALL) {
        return false;
    }
    return true;
}

std::string StaticLevel::toString() const {
    std::stringstream ss;
    ss << domain_ << ", " << name_ << ", " << walls_.size_rows() << "x" << walls_.size_cols();
    return ss.str();
}

Level::Level(StaticLevel static_level, std::vector<Agent> agents) : static_level(static_level), agents(agents) {}

std::string Level::toString() const {
    std::stringstream ss;
    ss << "Level(" << static_level.toString() << ", " << agents.size() << ")";
    return ss.str();
}

Level loadLevel(std::istream &serverMessages) {
    // Read domain (skip)
    std::string line;
    getline(serverMessages, line);  // #domain
    assert(line == "#domain");
    getline(serverMessages, line);  // hospital
    std::string domain = line;

    // Read level name (skip)
    getline(serverMessages, line);  // #levelname
    assert(line == "#levelname");
    getline(serverMessages, line);  // <name>
    std::string name = line;

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

    std::map<char, Color> agent_colors;
    std::map<char, Color> box_colors;

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
                agent_colors.insert({entityChar, color});
                continue;
            }

            if (FIRST_BOX <= entityChar && entityChar <= LAST_BOX) {
                box_colors.insert({entityChar, color});
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

    const size_t numRows = levelLines.size();
    const size_t numCols = std::max_element(levelLines.begin(), levelLines.end(), [](const std::string &a, const std::string &b) {
                               return a.length() < b.length();
                           })->length();

    CharGrid walls(numRows, numCols);
    // Level::box_goals = CharGrid(numRows, numCols);
    // std::vector<Cell2D> agents(LAST_AGENT - FIRST_AGENT + 1);
    std::vector<Agent> agents;
    agents.reserve(LAST_AGENT - FIRST_AGENT + 1);

    std::map<char, Cell2D> agent_positions;
    std::map<char, std::vector<Cell2D>> agent_goals;
    // std::unordered_map<Color, CharGrid> boxes_map;
    // for (int i = FIRST_BOX; i <= LAST_BOX; i++) {
    //     boxes_map.insert({Level::box_colors[i - FIRST_BOX], CharGrid(numRows, numCols)});
    // }

    for (size_t row = 0; row < numRows; row++) {
        const std::string &line = levelLines[row];
        for (size_t col = 0; col < line.length(); col++) {
            const char c = line[col];
            if (FIRST_AGENT <= c && c <= LAST_AGENT) {
                agent_positions[c] = Cell2D(row, col);
                continue;
            }
            if (FIRST_BOX <= c && c <= LAST_BOX) {
                // boxes_map[Level::box_colors[c - FIRST_BOX]](row, col) = c;
                continue;
            }
            if (c == WALL) {
                walls(row, col) = c;
                continue;
            }
            if (c == EMPTY) {
                continue;
            }

            fprintf(stderr, "Unknown character while reading initial state: %c\n", c);
        }
    }

    // Read goal state
    std::vector<std::string> goalLines;
    getline(serverMessages, line);  // first line of goal state
    while (line.find("#") == std::string::npos) {
        goalLines.push_back(line);
        getline(serverMessages, line);
    }

    for (size_t row = 0; row < numRows; row++) {
        const std::string &line = goalLines[row];
        for (size_t col = 0; col < line.length(); col++) {
            char c = line[col];
            if (FIRST_AGENT <= c && c <= LAST_AGENT) {
                agent_goals[c].push_back(Cell2D(row, col));
                continue;
            }
            if (FIRST_BOX <= c && c <= LAST_BOX) {
                // Level::box_goals(row, col) = c;
                continue;
            }
            if (c == WALL) {
                continue;
            }
            if (c == EMPTY) {
                continue;
            }

            fprintf(stderr, "Unknown character while reading goal state: %c\n", c);
        }
    }

    for (const auto &[agent_char, agent_color] : agent_colors) {
        agents.push_back(Agent(agent_positions[agent_char], agent_goals[agent_char], agent_color, agent_char));
    }

    return Level(StaticLevel(name, domain, walls, agent_colors, box_colors), agents);
}

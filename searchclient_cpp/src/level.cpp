#include "level.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <string_view>
#include <algorithm>

#include "helpers.hpp"
#include "goal.hpp"

// Constructor implementation
Level::Level(const std::unordered_map<char, Agent> &initialAgentsMap,
             const std::unordered_map<char, Box> &initialBoxesMap,
             std::vector<std::vector<bool>> walls,
             std::unordered_map<char, Goal> goalsMap,
             std::map<char, Point2D> goalsMap_for_Point2D,
             std::string domain, std::string name, int rows, int cols)
    : walls_(std::move(walls)),
      goalsMap_(std::move(goalsMap)),
      goalsMap_for_Point2D_(std::move(goalsMap_for_Point2D)),
      domain_(std::move(domain)),
      name_(std::move(name)),
      rows_(rows),
      cols_(cols),
      agentsMap(initialAgentsMap),
      boxesMap(initialBoxesMap) {}

std::string Level::toString() {
    std::stringstream ss;
    ss << "Level(" << this->domain_ << ", " << this->name_ << ", " << this->rows_ << "x" << this->cols_ << ")";
    return ss.str();
}

bool Level::isWall(int r_y, int c_x) const {
    if (r_y < 0 || r_y >= this->rows_ || c_x < 0 || c_x >= this->cols_) {
        return true; // Treat out of bounds as walls
    }
    // Ensure walls_ itself is not empty and the row is valid before accessing
    if (this->walls_.empty() || static_cast<size_t>(r_y) >= this->walls_.size() || this->walls_[static_cast<size_t>(r_y)].empty()) {
        return true;
    }
    return this->walls_[static_cast<size_t>(r_y)][static_cast<size_t>(c_x)];
}

Point2D Level::getGoalPosition(char entity_id) const {
    auto it = this->goalsMap_for_Point2D_.find(entity_id);
    if (it != this->goalsMap_for_Point2D_.end()) {
        return it->second;
    }
    return Point2D(-1, -1); // Indicates no specific goal found / invalid
}

// Global loadLevel function remains largely the same but populates instance members
// of the returned Level object.
Level loadLevel(std::istream &serverMessages) {
    std::string line;
    std::string current_domain;
    std::string current_name;
    std::vector<std::vector<bool>> parsed_walls_data;
    std::unordered_map<char, Goal> parsed_goals_map_data;
    std::map<char, Point2D> parsed_goals_map_for_point2d_data;
    int parsed_rows = 0;
    int parsed_cols = 0;

    // Read domain
    getline(serverMessages, line); // #domain
    // assert(line == "#domain"); // Can be too strict if server format varies slightly
    getline(serverMessages, line); // hospital
    current_domain = line;

    // Read level name
    getline(serverMessages, line); // #levelname
    // assert(line == "#levelname");
    getline(serverMessages, line); // <name>
    current_name = line;

    // Read colors
    getline(serverMessages, line); // #colors
    // assert(line == "#colors");
    getline(serverMessages, line);

    std::vector<std::string> colorSection;
    colorSection.reserve(10);
    while (line.find("#") == std::string::npos && !serverMessages.eof()) {
        colorSection.push_back(utils::trim(line));
        getline(serverMessages, line);
    }

    std::unordered_map<char, Color> agentColors;
    std::unordered_map<char, Color> boxColors;

    for (const std::string &color_line : colorSection) {
        std::string colorStr, entitiesStr;
        std::stringstream ss(color_line);
        getline(ss, colorStr, ':');
        getline(ss, entitiesStr);
        const Color color = from_string(colorStr);
        std::stringstream entitiesStream(entitiesStr);
        std::string entity;
        while (getline(entitiesStream, entity, ',')) {
            entity = utils::normalizeWhitespace(entity);
            entity = utils::trim(entity);
            if (entity.length() != 1) continue;
            const char entityChar = entity[0];
            if (FIRST_AGENT <= entityChar && entityChar <= LAST_AGENT) agentColors.insert({entityChar, color});
            if (FIRST_BOX <= entityChar && entityChar <= LAST_BOX) boxColors.insert({entityChar, color});
        }
    }

    // Read initial state section for agents and boxes (to construct the Level object)
    // The actual State object is built later in searchclient.cpp using this Level's maps
    std::vector<std::string> initial_layout_lines;
    initial_layout_lines.reserve(100);
    // line should currently hold "#initial" or be at EOF if colors were last
    if (line != "#initial") { // In case color section was missing or ended abruptly
        while(getline(serverMessages, line) && line.find("#initial") == std::string::npos && !serverMessages.eof());
    }
    // assert(line == "#initial");

    getline(serverMessages, line); // First line of initial state layout
    while (line.find("#") == std::string::npos && !serverMessages.eof()) {
        initial_layout_lines.push_back(line);
        getline(serverMessages, line);
    }

    parsed_rows = initial_layout_lines.size();
    if (!initial_layout_lines.empty()) {
        parsed_cols = std::max_element(initial_layout_lines.begin(), initial_layout_lines.end(),
                                     [](const std::string &a, const std::string &b) {
                                         return a.length() < b.length();
                                     })->length();
    }

    std::unordered_map<char, Agent> parsed_agents_map_for_level_constructor;
    std::unordered_map<char, Box> parsed_boxes_map_for_level_constructor;
    parsed_walls_data.resize(parsed_rows, std::vector<bool>(parsed_cols, false));

    for (int r = 0; r < parsed_rows; ++r) {
        const std::string &layout_line = initial_layout_lines[r];
        for (int c = 0; c < static_cast<int>(layout_line.length()); ++c) {
            if (c >= parsed_cols) break; // Should not happen if parsed_cols is correct
            const char entity_char = layout_line[c];
            if (entity_char == '+') {
                parsed_walls_data[r][c] = true;
            } else if (FIRST_AGENT <= entity_char && entity_char <= LAST_AGENT) {
                parsed_agents_map_for_level_constructor.insert({entity_char, Agent(entity_char, c, r, agentColors[entity_char])});
            } else if (FIRST_BOX <= entity_char && entity_char <= LAST_BOX) {
                parsed_boxes_map_for_level_constructor.insert({entity_char, Box(entity_char, c, r, boxColors[entity_char])});
            }
        }
    }
    
    // In the original MA-MA niveaux, #map section defines the walls explicitly.
    // If #initial section is used for walls, ensure it is comprehensive.
    // The provided log showed server sending #initial then #goal, with #map potentially missing or implicit.
    // For now, let's assume walls are fully defined by '+' in #initial, or a #map section would follow.
    // If there's an explicit #map section, we might need to reconcile or prioritize.
    // The current line variable should hold "#goal" or similar if #initial was processed.

    // Read goal state
    std::vector<std::string> goal_layout_lines;
    // assert(line == "#goal");
     if (line != "#goal") { // Ensure we are at goal section
        while(getline(serverMessages, line) && line.find("#goal") == std::string::npos && !serverMessages.eof());
    }

    getline(serverMessages, line); // First line of goal layout
    while (line.find("#") == std::string::npos && line != "#end" && !serverMessages.eof()) {
        goal_layout_lines.push_back(line);
        getline(serverMessages, line);
    }

    // Update rows/cols if goal layout is larger (though typically map/initial defines max dimensions)
    if (goal_layout_lines.size() > static_cast<size_t>(parsed_rows)) {
        // This might indicate an issue or a format where goal defines part of map size.
        // For now, we stick to dimensions from initial/map.
    }
    for (const auto& gl_line : goal_layout_lines) {
        if (gl_line.length() > static_cast<size_t>(parsed_cols)) {
            // parsed_cols = gl_line.length();
        }
    }
    // Ensure walls_data has correct dimensions if updated by goal section (not typical)
    // parsed_walls_data.resize(parsed_rows, std::vector<bool>(parsed_cols, false));

    for (size_t r = 0; r < goal_layout_lines.size(); ++r) {
        const std::string &current_g_line = goal_layout_lines[r];
        for (size_t c = 0; c < current_g_line.length(); ++c) {
            if (r >= static_cast<size_t>(parsed_rows) || c >= static_cast<size_t>(parsed_cols)) continue; // Goal outside defined map
            char entity_char = current_g_line[c];
            if ((FIRST_BOX <= entity_char && entity_char <= LAST_BOX) ||
                (FIRST_AGENT <= entity_char && entity_char <= LAST_AGENT)) {
                parsed_goals_map_data.insert({entity_char, Goal(entity_char, static_cast<int>(c), static_cast<int>(r))});
                parsed_goals_map_for_point2d_data[entity_char] = Point2D(static_cast<int>(c), static_cast<int>(r));
            }
        }
    }

    return Level(parsed_agents_map_for_level_constructor, 
                 parsed_boxes_map_for_level_constructor,
                 parsed_walls_data, 
                 parsed_goals_map_data, 
                 parsed_goals_map_for_point2d_data, 
                 current_domain, 
                 current_name, 
                 parsed_rows, 
                 parsed_cols);
}

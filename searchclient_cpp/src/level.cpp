#include "level.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <string_view>
#include <algorithm>
#include <cstdlib>

#include "helpers.hpp"
#include "goal.hpp"
#include "color.hpp"

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
      boxesMap(initialBoxesMap) {
        // Populate sorted IDs after maps are initialized
        for (const auto& pair : agentsMap) { sortedAgentIds_.push_back(pair.first); }
        std::sort(sortedAgentIds_.begin(), sortedAgentIds_.end());
        for (const auto& pair : boxesMap) { sortedBoxIds_.push_back(pair.first); }
        std::sort(sortedBoxIds_.begin(), sortedBoxIds_.end());
      }

std::string Level::toString() const {
    std::stringstream ss;
    ss << "Level(" << this->domain_ << ", " << this->name_ << ", " << this->rows_ << "x" << this->cols_ << ")";
    return ss.str();
}

bool Level::isWall(int r_y, int c_x) const {
    if (r_y < 0 || r_y >= this->rows_ || c_x < 0 || c_x >= this->cols_) {
        return true; // Treat out of bounds as walls
    }
    if (this->walls_.empty() || static_cast<size_t>(r_y) >= this->walls_.size() || this->walls_[static_cast<size_t>(r_y)].empty() || static_cast<size_t>(c_x) >= this->walls_[static_cast<size_t>(r_y)].size()) {
         // Added check for column bounds within the specific row vector
        fprintf(stderr, "Warning: isWall check out of bounds for valid row %d (max %zu) or col %d (max %zu for row). Level: %s\n", 
                r_y, this->walls_.size() > 0 ? this->walls_.size()-1 : 0, 
                c_x, (this->walls_.size() > 0 && static_cast<size_t>(r_y) < this->walls_.size()) ? (this->walls_[static_cast<size_t>(r_y)].size() > 0 ? this->walls_[static_cast<size_t>(r_y)].size()-1 : 0) : 0,
                this->toString().c_str());
        return true; // Should be an error or ensure Level construction prevents this.
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

Level loadLevel(std::istream &serverMessages) {
    std::string line;
    std::string current_domain;
    std::string current_name;
    std::vector<std::vector<bool>> parsed_walls_data;
    std::unordered_map<char, Goal> parsed_goals_map_data;
    std::map<char, Point2D> parsed_goals_map_for_point2d_data;
    std::unordered_map<char, Agent> parsed_agents_map_for_level_constructor;
    std::unordered_map<char, Box> parsed_boxes_map_for_level_constructor;
    
    int parsed_rows = 0;
    int parsed_cols = 0;

    auto findSection = [&](const std::string& section_name, bool is_optional = false) -> bool {
        if (serverMessages.good() && line.rfind(section_name, 0) == 0) { 
            return true;
        }
        line.clear(); 
        serverMessages.clear(); // Ensure stream is in a good state before this loop
        while (getline(serverMessages, line)) {
            if (line.rfind(section_name, 0) == 0) {
                return true;
            }
            if (!line.empty() && line[0] != '#') { // Allow full-line comments
                 // fprintf(stderr, "Warning: Unexpected content while seeking for %s: %s\n", section_name.c_str(), line.c_str());
            }
        }
        if (!is_optional) {
            fprintf(stderr, "Error: Required section header '%s' not found. EOF reached or stream error.\n", section_name.c_str());
            std::exit(EXIT_FAILURE); // Exit if mandatory section header not found
        }
        return false; 
    };
    
    // Read #domain
    if (!findSection("#domain")) { /* Already exited if not found */ }
    {
        std::string domain_value_line; // Local string for this specific getline
        serverMessages.clear(); // Clear any error flags on the stream
        if (!getline(serverMessages, domain_value_line) || domain_value_line.empty()) { 
            fprintf(stderr, "Error: Failed to read #domain value or it's empty.\n"); std::exit(EXIT_FAILURE); 
        }
        current_domain = utils::trim(domain_value_line);
    }
    if (current_domain.empty()){
        fprintf(stderr, "Error: #domain value is empty after trim.\n"); std::exit(EXIT_FAILURE);
    }


    // Read #levelname
    // 'line' will hold the next line after domain value, or findSection will read.
    if (!findSection("#levelname")) { /* Already exited */ }
    {
        std::string levelname_value_line; // Local string for this specific getline
        serverMessages.clear(); // Clear any error flags on the stream
        if (!getline(serverMessages, levelname_value_line) || levelname_value_line.empty()) { 
            fprintf(stderr, "Error: Failed to read #levelname value or it's empty.\n"); std::exit(EXIT_FAILURE); 
        }
        current_name = utils::trim(levelname_value_line);
    }
     if (current_name.empty()){
        fprintf(stderr, "Error: #levelname value is empty after trim.\n"); std::exit(EXIT_FAILURE);
    }

    // Read #colors (optional)
    std::vector<std::string> colorSection_lines;
    if (findSection("#colors", true /* is_optional */)) {
        while (getline(serverMessages, line)) {
            if (line.empty() || line.rfind('#', 0) == 0) break; 
            colorSection_lines.push_back(utils::trim(line));
        }
    } // else: #colors is optional, line will be the header of the next section or empty if EOF

    std::unordered_map<char, Color> agentColors;
    std::unordered_map<char, Color> boxColors;
    for (const std::string &color_line_str : colorSection_lines) {
        if (color_line_str.empty()) continue;
        std::string colorStr, entitiesStr;
        std::stringstream ss(color_line_str);
        getline(ss, colorStr, ':');
        colorStr = utils::trim(colorStr);
        getline(ss, entitiesStr); // Read the rest
        entitiesStr = utils::trim(entitiesStr);

        if (colorStr.empty() || entitiesStr.empty()) { 
            fprintf(stderr, "Warning: Malformed color line: '%s'. Skipping.\n", color_line_str.c_str());
            continue; 
        }
        const Color color = from_string(colorStr); // from_string handles unknown colors
        
        std::string entity;
        std::stringstream entitiesStream(entitiesStr);
        while (getline(entitiesStream, entity, ',')) {
            entity = utils::trim(entity);
            if (entity.length() != 1) {
                 fprintf(stderr, "Warning: Invalid entity '%s' in color line '%s'. Skipping entity.\n", entity.c_str(), color_line_str.c_str());
                continue;
            }
            const char entityChar = entity[0];
            if (FIRST_AGENT <= entityChar && entityChar <= LAST_AGENT) agentColors[entityChar] = color;
            else if (FIRST_BOX <= entityChar && entityChar <= LAST_BOX) boxColors[entityChar] = color;
            else {
                fprintf(stderr, "Warning: Unknown entity type char '%c' in color line '%s'. Skipping entity.\n", entityChar, color_line_str.c_str());
            }
        }
    }
    
    // Read #initial (mandatory)
    if (!findSection("#initial")) { /* Already exited */ }
    std::vector<std::string> initial_layout_lines;
    initial_layout_lines.reserve(100); 
    serverMessages.clear(); // Clear any EOF or error flags before reading initial layout
    { // New scope for temp_line for reading initial content
        std::string temp_line;
        while (getline(serverMessages, temp_line)) {
            //fprintf(stderr, "DEBUG_LEVEL_LOAD: Read initial line: '%s' (length %zu)\n", temp_line.c_str(), temp_line.length());
            if (temp_line.empty() || temp_line.rfind('#', 0) == 0) {
                line = temp_line; // Store the break-causing line (e.g. "#goal") in the outer 'line'
                //fprintf(stderr, "DEBUG_LEVEL_LOAD: Initial loop breaking on line: '%s'\n", line.c_str());
                break; 
            }
            initial_layout_lines.push_back(temp_line); 
        }
        // If getline failed (not just EOF), 'line' (outer) retains its value from findSection (e.g. "#initial")
        // or from a previous successful read if the loop ran at least once.
        // If the stream is just at EOF, 'line' would hold the last successfully processed header.
    } // End scope for temp_line

    if (initial_layout_lines.empty()) {
        fprintf(stderr, "Error: #initial section is empty.\n"); std::exit(EXIT_FAILURE);
    }

    parsed_rows = initial_layout_lines.size();
    for(const auto& l : initial_layout_lines) {
        if (static_cast<int>(l.length()) > parsed_cols) {
            parsed_cols = l.length();
        }
    }
    if (parsed_rows <= 0 || parsed_cols <= 0) {
        fprintf(stderr, "Error: Invalid level dimensions after #initial (rows: %d, cols: %d).\n", parsed_rows, parsed_cols); 
        std::exit(EXIT_FAILURE);
    }

    parsed_walls_data.resize(parsed_rows, std::vector<bool>(parsed_cols, false));
    for (int r = 0; r < parsed_rows; ++r) {
        const std::string &current_layout_line = initial_layout_lines[r];
        for (int c = 0; c < static_cast<int>(current_layout_line.length()); ++c) {
            // No need to check c against parsed_cols, as vector is sized and default false
            const char entity_char = current_layout_line[c];
            if (entity_char == '+') {
                parsed_walls_data[r][c] = true;
            } else if (FIRST_AGENT <= entity_char && entity_char <= LAST_AGENT) {
                Color color = Color::Blue; // Default
                if (agentColors.count(entity_char)) color = agentColors[entity_char];
                else fprintf(stderr, "Warning: Agent '%c' in #initial has no color specified in #colors. Using default.\n", entity_char);
                parsed_agents_map_for_level_constructor.emplace(entity_char, Agent(entity_char, c, r, color));
            } else if (FIRST_BOX <= entity_char && entity_char <= LAST_BOX) {
                Color color = Color::Blue; // Default
                if (boxColors.count(entity_char)) color = boxColors[entity_char];
                else fprintf(stderr, "Warning: Box '%c' in #initial has no color specified in #colors. Using default.\n", entity_char);
                parsed_boxes_map_for_level_constructor.emplace(entity_char, Box(entity_char, c, r, color));
            } else if (entity_char != ' ') {
                 fprintf(stderr, "Warning: Unrecognized character '%c' at (%d,%d) in #initial section. Treating as empty space.\n", entity_char, r, c);
            }
        }
    }

    // Read #goal (mandatory)
    if (!findSection("#goal")) { /* Already exited */ }
    std::vector<std::string> goal_layout_lines;
    goal_layout_lines.reserve(100);
    serverMessages.clear(); // Clear any EOF or error flags before reading goal layout
    { // New scope for temp_line for reading goal content
        std::string temp_line;
        while (getline(serverMessages, temp_line)) {
            if (temp_line.empty() || temp_line.rfind('#', 0) == 0) {
                line = temp_line; // Store break-causing line (e.g. "#map" or other) in outer 'line'
                break;
            }
            goal_layout_lines.push_back(temp_line);
        }
        // Similar logic for 'line' (outer) as in the #initial block.
    } // End scope for temp_line
    
    if (goal_layout_lines.empty()) {
        fprintf(stderr, "Error: #goal section is empty.\n"); std::exit(EXIT_FAILURE);
    }

    for (size_t r = 0; r < goal_layout_lines.size(); ++r) {
        const std::string &current_g_line = goal_layout_lines[r];
        for (size_t c = 0; c < current_g_line.length(); ++c) {
            if (r >= static_cast<size_t>(parsed_rows) || c >= static_cast<size_t>(parsed_cols)) {
                 fprintf(stderr, "Warning: Goal specification at (%zu,%zu) is outside of initial map dimensions (%d,%d). Ignoring.\n", r,c, parsed_rows, parsed_cols);
                continue;
            }
            char entity_char = current_g_line[c];
            if ((FIRST_BOX <= entity_char && entity_char <= LAST_BOX) ||
                (FIRST_AGENT <= entity_char && entity_char <= LAST_AGENT)) {
                // This character is a valid agent or box ID, process it as a goal.
                if (parsed_goals_map_data.count(entity_char)) {
                     fprintf(stderr, "Warning: Duplicate goal for entity '%c' at (%zu,%zu). Previous definition at (%d,%d) will be overwritten.\n", 
                             entity_char, c, r, 
                             parsed_goals_map_for_point2d_data.count(entity_char) ? parsed_goals_map_for_point2d_data.at(entity_char).x() : -1, 
                             parsed_goals_map_for_point2d_data.count(entity_char) ? parsed_goals_map_for_point2d_data.at(entity_char).y() : -1);
                     parsed_goals_map_data.erase(entity_char); // Erase before emplace for overwrite
                }
                parsed_goals_map_data.emplace(entity_char, Goal(entity_char, static_cast<int>(c), static_cast<int>(r)));
                
                parsed_goals_map_for_point2d_data.insert_or_assign(entity_char, Point2D(static_cast<int>(c), static_cast<int>(r)));
            } // else: any other character (space, wall '+', etc.) in the #goal section is silently ignored.
        }
    }

    // Read #map (optional) - currently just skips if found
    if (findSection("#map", true /*is_optional*/)) {
        // Future: could parse map section and validate/compare with initial layout
        // For now, consume lines until next section or EOF
        while(getline(serverMessages, line)) {
            if (line.empty() || line.rfind('#', 0) == 0) break;
        }
    }
    
    // All mandatory sections parsed and validated, dimensions are positive.
    Level final_level_obj(parsed_agents_map_for_level_constructor,
                         parsed_boxes_map_for_level_constructor,
                         parsed_walls_data,
                         parsed_goals_map_data,
                         parsed_goals_map_for_point2d_data,
                         current_domain,
                         current_name,
                         parsed_rows,
                         parsed_cols);
    
    // The sortedAgentIds_ and sortedBoxIds_ are now populated in the Level constructor.
    
    return final_level_obj;
}

// Helper to find max line length, ensures parsed_cols is correctly set
// No longer needed as it's integrated into the main loop for initial_layout_lines.
// int getMaxLineLength(const std::vector<std::string>& lines) { ... }

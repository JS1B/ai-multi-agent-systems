#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <limits>
#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "join.hpp"
#include "state.h"
#include "frontier.h"
#include "graphsearch.h"
#include "action.h"
#include "color.h"
#include "heuristic.h"

State parseLevel(std::istream &serverMessages)
{
    // Read domain (skip)
    std::string line;
    getline(serverMessages, line); // #domain
    getline(serverMessages, line); // hospital

    // Read level name (skip)
    getline(serverMessages, line); // #levelname
    getline(serverMessages, line); // <name>

    // Read colors
    getline(serverMessages, line); // #colors
    std::vector<Color> agentColors(10, Color::None);
    std::vector<Color> boxColors(26, Color::None);

    getline(serverMessages, line);
    while (line.find("#") == std::string::npos)
    {
        std::stringstream ss(line);
        std::string colorStr, entitiesStr;
        getline(ss, colorStr, ':');
        getline(ss, entitiesStr);

        Color color = colorFromString(colorStr);

        std::stringstream entitiesStream(entitiesStr);
        std::string entity;
        while (getline(entitiesStream, entity, ','))
        {
            entity.erase(std::remove_if(entity.begin(), entity.end(), (int (*)(int))std::isspace), entity.end()); // Trim whitespace
            if (entity.length() == 1)
            {
                char c = entity[0];
                if ('0' <= c && c <= '9')
                {
                    agentColors[c - '0'] = color;
                }
                else if ('A' <= c && c <= 'Z')
                {
                    boxColors[c - 'A'] = color;
                }
            }
        }
        getline(serverMessages, line);
    }

    // Read initial state
    int numRows = 0;
    int numCols = 0;
    std::vector<std::string> levelLines;

    getline(serverMessages, line); // #initial
    getline(serverMessages, line);
    while (line.find("#") == std::string::npos)
    {
        levelLines.push_back(line);
        numCols = std::max(numCols, (int)line.length());
        numRows++;
        getline(serverMessages, line);
    }

    int numAgents = 0;
    std::vector<int> agentRows(10, -1);
    std::vector<int> agentCols(10, -1);
    std::vector<std::vector<bool>> walls(numRows, std::vector<bool>(numCols, false));
    std::vector<std::vector<char>> boxes(numRows, std::vector<char>(numCols, ' '));

    for (int row = 0; row < numRows; ++row)
    {
        for (size_t col = 0; col < levelLines[row].length(); ++col)
        {
            char c = levelLines[row][col];
            if ('0' <= c && c <= '9')
            {
                int agentNum = c - '0';
                agentRows[agentNum] = row;
                agentCols[agentNum] = col;
                numAgents++;
            }
            else if ('A' <= c && c <= 'Z')
            {
                boxes[row][col] = c;
            }
            else if (c == '+')
            {
                walls[row][col] = true;
            }
        }
    }

    agentRows.resize(numAgents);
    agentCols.resize(numAgents);

    // Read goal state
    std::vector<std::vector<char>> goals(numRows, std::vector<char>(numCols, ' '));
    getline(serverMessages, line); // #goal
    getline(serverMessages, line);
    int row = 0;
    while (line.find("#") == std::string::npos)
    {
        for (size_t col = 0; col < line.length(); ++col)
        {
            char c = line[col];
            if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z'))
            {
                goals[row][col] = c;
            }
        }
        row++;
        getline(serverMessages, line);
    }

    State::agentColors = agentColors;
    State::walls = walls;
    State::boxColors = boxColors;
    // State::goals = goals;

    return State(agentRows, agentCols, agentColors, walls, boxes, boxColors, goals);
}

int main(int argc, char *argv[])
{
    // Use stderr to print to the console.
    fprintf(stderr, "C++ SearchClient initializing. I am sending this using the error output stream.\n");

    // Send client name to server.
    fprintf(stdout, "SearchClient\n");

    // We can also print comments to stdout by prefixing with a #.
    fprintf(stdout, "#This is a comment.\n");

    // Parse command line arguments (e.g., for specifying search strategy)
    std::string strategy = "bfs";
    if (argc > 1)
    {
        strategy = argv[1];
    }

    // Parse the level from stdin
    State initial_state = parseLevel(std::cin);

    // Create frontier
    Frontier *frontier;
    if (strategy == "bfs")
    {
        frontier = new FrontierBFS();
    }
    else if (strategy == "dfs")
    {
        frontier = new FrontierDFS();
    }
    else if (strategy == "astar")
    {
        frontier = new FrontierBestFirst(new HeuristicAStar(initial_state));
    }
    else if (strategy == "wastar")
    {
        int w = 5; // Default weight
        if (argc > 2)
        {
            try
            {
                w = std::stoi(argv[2]);
            }
            catch (const std::invalid_argument &e)
            {
                fprintf(stderr, "Invalid weight for WA*: %s\n", argv[2]);
                return 1;
            }
        }
        frontier = new FrontierBestFirst(new HeuristicWeightedAStar(initial_state, w));
    }
    else if (strategy == "greedy")
    {
        frontier = new FrontierBestFirst(new HeuristicGreedy(initial_state));
    }
    else
    {
        fprintf(stderr, "Unknown strategy: %s\n", strategy.c_str());
        return 1;
    }

    // Search for a plan
    fprintf(stderr, "Starting %s.\n", frontier->getName().c_str());
    std::vector<std::vector<Action>> plan = search(initial_state, frontier);

    // Print plan to server
    if (plan.empty())
    {
        fprintf(stderr, "Unable to solve level.\n");
        return 0;
    }
    else
    {
        fprintf(stdout, "Found solution of length %zu.\n", plan.size());
        for (const auto &joint_action : plan)
        {
            std::vector<std::string> actionNames;
            for (const auto &action : joint_action)
            {
                actionNames.push_back(action.name + "@" + action.name);
            }
            std::string s = utils::join(actionNames, "|");
            fprintf(stdout, "%s\n", s.c_str());
            fflush(stdout);

            // Read server's response to not fill up the stdin buffer and block the server.
            std::string response;
            std::getline(std::cin, response);
        }
    }

    delete frontier;
    return 0;
}
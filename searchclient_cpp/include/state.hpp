#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <tuple>  // For lexicographical comparison
#include <vector>

#include "action.hpp"
#include "color.hpp"

class State {
   public:
    State() = default;

    std::vector<int> agentRows;
    std::vector<int> agentCols;
    std::vector<Color> agentColors;

    std::vector<std::vector<bool>> walls;
    std::vector<std::vector<char>> boxes;
    std::vector<Color> boxColors;
    std::vector<std::vector<char>> goals;

    const State *parent;
    std::vector<Action> jointAction;

    State(std::vector<int> agentRows, std::vector<int> agentCols, std::vector<Color> agentColors, std::vector<std::vector<bool>> walls,
          std::vector<std::vector<char>> boxes, std::vector<Color> boxColors, std::vector<std::vector<char>> goals)
        : agentRows(agentRows),
          agentCols(agentCols),
          agentColors(agentColors),
          walls(walls),
          boxes(boxes),
          boxColors(boxColors),
          goals(goals),
          parent(nullptr),
          g_(0) {};

    inline int getG() const { return g_; }

    std::vector<std::vector<Action>> extractPlan() {
        std::vector<std::vector<Action>> plan;
        const State *current = this;

        while (current->parent != nullptr) {
            // Use push_back which uses copy construction
            plan.push_back(current->jointAction);
            current = current->parent;
        }
        // The plan is currently in reverse order, reverse it
        std::reverse(plan.begin(), plan.end());
        return plan;
    }

    std::vector<State> getExpandedStates() const {
        std::vector<State> expandedStates;

        size_t numAgents = agentRows.size();
        std::vector<std::vector<Action>> applicableAction(numAgents);

        for (const Action *actionPtr : Action::values()) {
            for (size_t agent = 0; agent < numAgents; ++agent) {
                if (!isApplicable(agent, *actionPtr)) {
                    continue;
                }
                applicableAction[agent].push_back(*actionPtr);
            }
        }

        std::vector<size_t> actionsPermutation(numAgents, 0);

        while (true) {
            std::vector<Action> currentJointAction;
            currentJointAction.reserve(numAgents);
            for (size_t agent = 0; agent < numAgents; ++agent) {
                currentJointAction.push_back(applicableAction[agent][actionsPermutation[agent]]);
            }

            if (!isConflicting(currentJointAction)) {
                expandedStates.push_back(State(*this, currentJointAction));
            }

            bool done = false;
            for (size_t agent = 0; agent < numAgents; ++agent) {
                if (actionsPermutation[agent] < applicableAction[agent].size() - 1) {
                    actionsPermutation[agent]++;
                    break;
                }
                actionsPermutation[agent] = 0;
                if (agent == numAgents - 1) done = true;
            }
            if (done) break;
        }

        return expandedStates;
    }

    inline bool isGoalState() const {
        for (size_t row = 1; row < goals.size() - 1; row++) {
            for (size_t col = 1; col < goals[row].size(); col++) {
                char goal = goals[row][col];
                if ('A' <= goal && goal <= 'Z' && boxes[row][col] != goal) return false;
                if ('0' <= goal && goal <= '9' && !(size_t(agentRows[goal - '0']) == row && size_t(agentCols[goal - '0']) == col))
                    return false;
            }
        }
        return true;
    }

    bool operator==(const State &other) const {
        if (this == &other) return true;

        if (agentRows.size() != other.agentRows.size() || agentCols.size() != other.agentCols.size() ||
            boxes.size() != other.boxes.size()) {
            return false;
        }

        if (agentRows != other.agentRows || agentCols != other.agentCols) {
            return false;
        }

        for (size_t i = 0; i < boxes.size(); ++i) {
            if (boxes[i] != other.boxes[i]) {
                return false;
            }
        }

        for (size_t i = 0; i < walls.size(); ++i) {
            if (walls[i] != other.walls[i]) {
                return false;
            }
        }

        for (size_t i = 0; i < goals.size(); ++i) {
            if (goals[i] != other.goals[i]) {
                return false;
            }
        }

        return true;
    }

    bool operator<(const State &other) const {
        // Compare based on agent positions first, then box positions
        // Using std::tie for easy lexicographical comparison
        return std::tie(agentRows, agentCols, boxes) < std::tie(other.agentRows, other.agentCols, other.boxes);
    }

    bool isApplicable(int agent, const Action &action) const {
        int agentRow = agentRows[agent];
        int agentCol = agentCols[agent];
        Color agentColor = agentColors[agent];
        int destinationRow, destinationCol;
        int boxRow, boxCol;
        char box;

        switch (action.type) {
            case ActionType::NoOp:
                return true;

            case ActionType::Move:
                destinationRow = agentRow + action.agentRowDelta;
                destinationCol = agentCol + action.agentColDelta;
                return cellIsFree(destinationRow, destinationCol);

            case ActionType::Push:
                boxRow = agentRow + action.boxRowDelta;
                boxCol = agentCol + action.boxColDelta;
                box = boxes[boxRow][boxCol];

                destinationRow = agentRow + action.agentRowDelta;
                destinationCol = agentCol + action.agentColDelta;
                return box != ' ' && agentColor == boxColors[box - 'A'] && cellIsFree(destinationRow, destinationCol);

            case ActionType::Pull:
                boxRow = agentRow - action.boxRowDelta;
                boxCol = agentCol - action.boxColDelta;
                box = boxes[boxRow][boxCol];

                destinationRow = agentRow + action.agentRowDelta;
                destinationCol = agentCol + action.agentColDelta;
                return box != ' ' && agentColor == boxColors[box - 'A'] && cellIsFree(destinationRow, destinationCol);

            default:
                throw std::invalid_argument("Invalid action type");
                // return false;
        }
    }

    bool isConflicting(const std::vector<Action> &jointAction) const {
        size_t numAgents = agentRows.size();

        int destinationRows[numAgents];
        int destinationCols[numAgents];

        for (size_t agent = 0; agent < numAgents; ++agent) {
            Action action = jointAction[agent];
            int agentRow = agentRows[agent];
            int agentCol = agentCols[agent];
            int boxRow, boxCol;

            switch (action.type) {
                case ActionType::NoOp:
                    break;

                case ActionType::Move:
                    destinationRows[agent] = agentRow + action.agentRowDelta;
                    destinationCols[agent] = agentCol + action.agentColDelta;
                    break;

                case ActionType::Push:
                    boxRow = agentRow + action.boxRowDelta;
                    boxCol = agentCol + action.boxColDelta;

                    destinationRows[agent] = boxRow + action.agentRowDelta;
                    destinationCols[agent] = boxCol + action.agentColDelta;
                    break;

                case ActionType::Pull:
                    boxRow = agentRow - action.boxRowDelta;
                    boxCol = agentCol - action.boxColDelta;

                    destinationRows[agent] = agentRow + action.agentRowDelta;
                    destinationCols[agent] = agentCol + action.agentColDelta;
                    break;

                default:
                    throw std::invalid_argument("Invalid action type");
                    // return false;
            }
        }

        for (size_t a1 = 0; a1 < numAgents; ++a1) {
            if (jointAction[a1].type == ActionType::NoOp) continue;

            for (size_t a2 = a1 + 1; a2 < numAgents; ++a2) {
                if (jointAction[a2].type == ActionType::NoOp) continue;

                if (destinationRows[a1] == destinationRows[a2] && destinationCols[a1] == destinationCols[a2]) {
                    return true;
                }
            }
        }
        return false;
    }

    std::string toString() {
        std::stringstream ss;
        ss << "State(agentRows=\n";
        for (size_t row = 0; row < walls.size(); row++) {
            for (size_t col = 0; col < walls[row].size(); col++) {
                if (boxes[row][col] != ' ')
                    ss << boxes[row][col];
                else if (walls[row][col])
                    ss << '+';
                else if (char x = agentAt(row, col); x != 0)
                    ss << x;
                else
                    ss << ' ';
            }
            ss << '\n';
        }
        return ss.str();
    }

    struct hash {
        size_t operator()(const State &s) const {
            size_t seed = 0;
            // Combine hashes of agent rows
            for (int row : s.agentRows) {
                seed ^= std::hash<int>()(row) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            // Combine hashes of agent cols
            for (int col : s.agentCols) {
                seed ^= std::hash<int>()(col) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            // Combine hashes of boxes (vector of vectors)
            for (const auto &rowVec : s.boxes) {
                for (char box : rowVec) {
                    seed ^= std::hash<char>()(box) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                }
            }
            return seed;
        }
    };

   private:
    // Creates a new state from a parent state and a joint action performed in that state.
    // This constructor should copy the parent state's data, not move it.
    State(const State &parent, std::vector<Action> jointAction)
        : agentRows(parent.agentRows),          // Copy instead of move
          agentCols(parent.agentCols),          // Copy instead of move
          agentColors(parent.agentColors),      // Copy (was implicit before, making explicit)
          walls(parent.walls),                  // Copy (was implicit before, making explicit)
          boxes(parent.boxes),                  // Copy instead of move
          boxColors(parent.boxColors),          // Copy (was implicit before, making explicit)
          goals(parent.goals),                  // Copy (was implicit before, making explicit)
          parent(&parent),                      // Store pointer to the parent
          jointAction(std::move(jointAction)),  // Move the joint action vector as it's unique to the child
          g_(parent.getG() + 1) {
        // The constructor body can remain empty if all initialization is done above.

        // Apply the joint action to the new state's data (agentRows, agentCols, boxes)
        size_t numAgents = agentRows.size();
        for (size_t agent = 0; agent < numAgents; ++agent) {
            const Action &action = this->jointAction[agent];  // Use this->jointAction

            switch (action.type) {
                case ActionType::NoOp:
                    break;

                case ActionType::Move:
                    agentRows[agent] += action.agentRowDelta;
                    agentCols[agent] += action.agentColDelta;
                    break;

                case ActionType::Push: {
                    int boxInitialRow = agentRows[agent] + action.boxRowDelta;
                    int boxInitialCol = agentCols[agent] + action.boxColDelta;
                    char box = boxes[boxInitialRow][boxInitialCol];
                    if (box != ' ') {  // Ensure there is a box to push
                        int boxDestinationRow = boxInitialRow + action.agentRowDelta;
                        int boxDestinationCol = boxInitialCol + action.agentColDelta;
                        boxes[boxDestinationRow][boxDestinationCol] = box;
                        boxes[boxInitialRow][boxInitialCol] = 0;  // Clear the original box position
                    }
                    // Agent moves to the initial box position
                    agentRows[agent] = boxInitialRow;
                    agentCols[agent] = boxInitialCol;
                    break;
                }

                case ActionType::Pull: {
                    int boxInitialRow = agentRows[agent] - action.boxRowDelta;  // Box starts behind agent
                    int boxInitialCol = agentCols[agent] - action.boxColDelta;
                    char box = boxes[boxInitialRow][boxInitialCol];
                    if (box != ' ') {  // Ensure there is a box to pull
                        int agentDestinationRow = agentRows[agent] + action.agentRowDelta;
                        int agentDestinationCol = agentCols[agent] + action.agentColDelta;
                        // Box moves to the agent's original position
                        boxes[agentRows[agent]][agentCols[agent]] = box;
                        boxes[boxInitialRow][boxInitialCol] = 0;  // Clear the original box position
                        // Agent moves to its destination
                        agentRows[agent] = agentDestinationRow;
                        agentCols[agent] = agentDestinationCol;
                    } else {
                        // If no box to pull, agent just moves
                        agentRows[agent] += action.agentRowDelta;
                        agentCols[agent] += action.agentColDelta;
                    }
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid action type");
            }
        }
    }

    int g_;  // cost of reaching this state

    bool cellIsFree(int row, int col) const { return !walls[row][col] && boxes[row][col] == ' ' && agentAt(row, col) == 0; }

    char agentAt(int row, int col) const {
        for (size_t i = 0; i < agentRows.size(); i++) {
            if (agentRows[i] == row && agentCols[i] == col) {
                return '0' + i;
            }
        }
        return 0;
    }
};

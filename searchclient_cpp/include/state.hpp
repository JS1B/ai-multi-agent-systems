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
                if (!isApplicable(agent, *actionPtr)) continue;
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
                return box != 0 && agentColor == boxColors[box - 'A'] && cellIsFree(destinationRow, destinationCol);

            case ActionType::Pull:
                boxRow = agentRow - action.boxRowDelta;
                boxCol = agentCol - action.boxColDelta;
                box = boxes[boxRow][boxCol];

                destinationRow = agentRow + action.agentRowDelta;
                destinationCol = agentCol + action.agentColDelta;
                return box != 0 && agentColor == boxColors[box - 'A'] && cellIsFree(destinationRow, destinationCol);

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
        ss << "State(agentRows=";
        for (size_t row = 0; row < walls.size(); row++) {
            for (size_t col = 0; col < walls[row].size(); col++) {
                if (boxes[row][col] > 0)
                    ss << boxes[row][col];
                else if (walls[row][col])
                    ss << '+';
                else if (agentAt(row, col) != 0)
                    ss << agentAt(row, col);
                else
                    ss << ' ';
            }
            ss << '\n';
        }
        return ss.str();
    }

   private:
    // Store the jointAction that led to this state
    State(const State &parent, std::vector<Action> jointAction)
        : parent(&parent),
          g_(parent.getG() + 1),
          jointAction(std::move(jointAction)),
          agentRows(std::move(parent.agentRows)),
          agentCols(std::move(parent.agentCols)),
          boxes(std::move(parent.boxes)) {
        // Apply the joint action to the copied state
        size_t numAgents = agentRows.size();
        for (size_t agent = 0; agent < numAgents; ++agent) {
            Action action = jointAction[agent];
            int agentRow = agentRows[agent];
            int agentCol = agentCols[agent];
            int boxRow, boxCol;
            char box;

            switch (action.type) {
                case ActionType::NoOp:
                    break;

                case ActionType::Move:
                    agentRows[agent] += action.agentRowDelta;
                    agentCols[agent] += action.agentColDelta;
                    break;

                case ActionType::Push:
                    boxRow = agentRow + action.boxRowDelta;
                    boxCol = agentCol + action.boxColDelta;
                    box = boxes[boxRow][boxCol];

                    agentRows[agent] += action.agentRowDelta;
                    agentCols[agent] += action.agentColDelta;

                    boxes[boxRow + action.boxRowDelta][boxCol + action.boxColDelta] = box;
                    boxes[boxRow][boxCol] = 0;
                    break;

                case ActionType::Pull:
                    boxRow = agentRow - action.boxRowDelta;
                    boxCol = agentCol - action.boxColDelta;
                    box = boxes[boxRow][boxCol];

                    agentRows[agent] += action.agentRowDelta;
                    agentCols[agent] += action.agentColDelta;

                    boxes[boxRow + action.boxRowDelta][boxCol + action.boxColDelta] = box;
                    boxes[boxRow][boxCol] = 0;
                    break;
            }
        }
    };

    const int g_;

    bool cellIsFree(int row, int col) const { return !walls[row][col] && boxes[row][col] == 0 && agentAt(row, col) == 0; }

    char agentAt(int row, int col) const {
        for (size_t i = 0; i < agentRows.size(); i++) {
            if (agentRows[i] == row && agentCols[i] == col) {
                return '0' + i;
            }
        }
        return 0;
    }
};

#pragma once

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string>
#include <tuple>  // For lexicographical comparison
#include <vector>

#include "action.hpp"
#include "color.hpp"
#include "level.hpp"

class State {
   public:
    State() = delete;
    State(Level level) : level(level), parent(nullptr), g_(0) {}  // Just to make sure the empty contructor remains deleted

    Level level;

    const State *parent;
    std::vector<Action> jointAction;

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

    std::vector<State *> getExpandedStates() const {
        std::vector<State *> expandedStates;

        size_t numAgents = level.agentRows.size();
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
            for (size_t agent = 0; agent < numAgents; ++agent) {
                currentJointAction.push_back(applicableAction[agent][actionsPermutation[agent]]);
            }

            if (!isConflicting(currentJointAction)) {
                expandedStates.push_back(new State(this, currentJointAction));
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
        for (size_t row = 1; row < level.goals.size() - 1; row++) {
            for (size_t col = 1; col < level.goals[row].size(); col++) {
                char goal = level.goals[row][col];
                if (FIRST_BOX <= goal && goal <= LAST_BOX && level.boxes[row][col] != goal) return false;
                if (FIRST_AGENT <= goal && goal <= LAST_AGENT &&
                    !(size_t(level.agentRows[goal - FIRST_AGENT]) == row && size_t(level.agentCols[goal - FIRST_AGENT]) == col))
                    return false;
            }
        }
        return true;
    }

    bool operator==(const State &other) const {
        if (this == &other) return true;

        if (level.agentRows.size() != other.level.agentRows.size() || level.agentCols.size() != other.level.agentCols.size() ||
            level.boxes.size() != other.level.boxes.size()) {
            return false;
        }

        if (level.agentRows != other.level.agentRows || level.agentCols != other.level.agentCols) {
            return false;
        }

        for (size_t i = 0; i < level.boxes.size(); ++i) {
            if (level.boxes[i] != other.level.boxes[i]) {
                return false;
            }
        }

        for (size_t i = 0; i < level.walls.size(); ++i) {
            if (level.walls[i] != other.level.walls[i]) {
                return false;
            }
        }

        for (size_t i = 0; i < level.goals.size(); ++i) {
            if (level.goals[i] != other.level.goals[i]) {
                return false;
            }
        }

        return true;
    }

    bool operator<(const State &other) const {
        // Compare based on agent positions first, then box positions
        // Using std::tie for easy lexicographical comparison
        return std::tie(level.agentRows, level.agentCols, level.boxes) <
               std::tie(other.level.agentRows, other.level.agentCols, other.level.boxes);
    }

    bool isApplicable(int agent, const Action &action) const {
        int agentRow = level.agentRows[agent];
        int agentCol = level.agentCols[agent];
        Color agentColor = level.agentColors[agent];
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
                box = level.boxes[boxRow][boxCol];

                destinationRow = agentRow + action.agentRowDelta;
                destinationCol = agentCol + action.agentColDelta;
                return box != EMPTY && agentColor == level.boxColors[box - FIRST_BOX] && cellIsFree(destinationRow, destinationCol);

            case ActionType::Pull:
                boxRow = agentRow - action.boxRowDelta;
                boxCol = agentCol - action.boxColDelta;
                box = level.boxes[boxRow][boxCol];

                destinationRow = agentRow + action.agentRowDelta;
                destinationCol = agentCol + action.agentColDelta;
                return box != EMPTY && agentColor == level.boxColors[box - FIRST_BOX] && cellIsFree(destinationRow, destinationCol);

            default:
                throw std::invalid_argument("Invalid action type");
                // return false;
        }
    }

    bool isConflicting(const std::vector<Action> &jointAction) const {
        size_t numAgents = level.agentRows.size();

        int destinationRows[numAgents];
        int destinationCols[numAgents];

        for (size_t agent = 0; agent < numAgents; ++agent) {
            Action action = jointAction[agent];
            int agentRow = level.agentRows[agent];
            int agentCol = level.agentCols[agent];
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
        for (size_t row = 0; row < level.walls.size(); row++) {
            for (size_t col = 0; col < level.walls[row].size(); col++) {
                if (level.boxes[row][col] != EMPTY)
                    ss << level.boxes[row][col];
                else if (level.walls[row][col])
                    ss << WALL;
                else if (char x = agentAt(row, col); x != 0)
                    ss << x;
                else
                    ss << EMPTY;
            }
            ss << '\n';
        }
        return ss.str();
    }

    struct hash {
        size_t operator()(const State *s) const {
            size_t seed = 0;
            // Combine hashes of agent rows
            for (int row : s->level.agentRows) {
                seed ^= std::hash<int>()(row) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            // Combine hashes of agent cols
            for (int col : s->level.agentCols) {
                seed ^= std::hash<int>()(col) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            // Combine hashes of boxes (vector of vectors)
            for (const auto &rowVec : s->level.boxes) {
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
    State(const State *parent, std::vector<Action> jointAction)
        : level(parent->level), parent(parent), jointAction(std::move(jointAction)), g_(parent->getG() + 1) {
        // Apply the joint action to the new state's data (agentRows, agentCols, boxes)
        size_t numAgents = level.agentRows.size();
        for (size_t agent = 0; agent < numAgents; ++agent) {
            const Action &action = this->jointAction[agent];  // Use this->jointAction

            switch (action.type) {
                case ActionType::NoOp:
                    break;

                case ActionType::Move:
                    level.agentRows[agent] += action.agentRowDelta;
                    level.agentCols[agent] += action.agentColDelta;
                    break;

                case ActionType::Push: {
                    int boxInitialRow = level.agentRows[agent] + action.boxRowDelta;
                    int boxInitialCol = level.agentCols[agent] + action.boxColDelta;
                    char box = level.boxes[boxInitialRow][boxInitialCol];
                    if (box != EMPTY) {
                        int boxDestinationRow = boxInitialRow + action.agentRowDelta;
                        int boxDestinationCol = boxInitialCol + action.agentColDelta;
                        level.boxes[boxDestinationRow][boxDestinationCol] = box;
                        level.boxes[boxInitialRow][boxInitialCol] = EMPTY;
                    }
                    // Agent moves to the initial box position
                    level.agentRows[agent] = boxInitialRow;
                    level.agentCols[agent] = boxInitialCol;
                    break;
                }

                case ActionType::Pull: {
                    int boxInitialRow = level.agentRows[agent] - action.boxRowDelta;  // Box starts behind agent
                    int boxInitialCol = level.agentCols[agent] - action.boxColDelta;
                    char box = level.boxes[boxInitialRow][boxInitialCol];
                    if (box != EMPTY) {
                        int agentDestinationRow = level.agentRows[agent] + action.agentRowDelta;
                        int agentDestinationCol = level.agentCols[agent] + action.agentColDelta;
                        // Box moves to the agent's original position
                        level.boxes[agentDestinationRow][agentDestinationCol] = box;
                        level.boxes[boxInitialRow][boxInitialCol] = EMPTY;
                        // Agent moves to its destination
                        level.agentRows[agent] = agentDestinationRow;
                        level.agentCols[agent] = agentDestinationCol;
                    } else {
                        // If no box to pull, agent just moves
                        level.agentRows[agent] += action.agentRowDelta;
                        level.agentCols[agent] += action.agentColDelta;
                    }
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid action type");
            }
        }
    }

    int g_;  // cost of reaching this state

    bool cellIsFree(int row, int col) const { return !level.walls[row][col] && level.boxes[row][col] == EMPTY && agentAt(row, col) == 0; }

    char agentAt(int row, int col) const {
        for (size_t i = 0; i < level.agentRows.size(); i++) {
            if (level.agentRows[i] == row && level.agentCols[i] == col) {
                return FIRST_AGENT + i;
            }
        }
        return 0;
    }
};

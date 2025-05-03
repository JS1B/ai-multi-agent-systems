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
    State(Level level) : level(level), parent(nullptr), g_(0) {}

    Level level;

    const State *parent;
    std::vector<const Action *> jointAction;

    inline int getG() const { return g_; }

    std::vector<std::vector<const Action *>> extractPlan() {
        std::vector<std::vector<const Action *>> plan;
        const State *current = this;

        while (current->parent != nullptr) {
            plan.push_back(current->jointAction);
            current = current->parent;
        }

        std::reverse(plan.begin(), plan.end());
        return plan;
    }

    std::vector<State *> getExpandedStates() const {
        std::vector<State *> expandedStates;

        size_t numAgents = level.agentsMap.size();
        std::vector<std::vector<const Action *>> applicableAction(numAgents);

        for (const Action *actionPtr : Action::allValues()) {
            for (const auto &agentPair : level.agentsMap) {
                if (!isApplicable(agentPair.second, *actionPtr)) {
                    continue;
                }
                applicableAction[agentPair.first].push_back(actionPtr);
            }
        }

        std::vector<size_t> actionsPermutation(numAgents, 0);

        while (true) {
            std::vector<const Action *> currentJointAction;
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

        // @todo: Shuffle the expanded states - why?
        // std::shuffle(expandedStates.begin(), expandedStates.end(), std::random_device());
        return expandedStates;
    }

    inline bool isGoalState() const {
        for (const auto &goal : level.goalsMap) {
            const char goalId = goal.first;
            const Point2D goalPos = goal.second.position();

            // Every box must be on a goal
            for (const auto &boxPair : level.boxesMap) {
                if (boxPair.second.getId() == goalId && !boxPair.second.at(goalPos)) return false;
            }

            // Every agent must be on a goal
            for (const auto &agentPair : level.agentsMap) {
                if (agentPair.second.getId() == goalId && !agentPair.second.at(goalPos)) return false;
            }
        }
        return true;
    }

    bool operator==(const State &other) const {
        if (this == &other) return true;

        if (level.agentsMap.size() != other.level.agentsMap.size() || level.boxesMap.size() != other.level.boxesMap.size() ||
            level.goalsMap.size() != other.level.goalsMap.size()) {
            return false;
        }

        if (level.agentsMap != other.level.agentsMap || level.boxesMap != other.level.boxesMap || level.goalsMap != other.level.goalsMap) {
            return false;
        }

        for (const auto &boxPair : level.boxesMap) {
            if (boxPair.second != other.level.boxesMap.at(boxPair.first)) {
                return false;
            }
        }

        for (size_t i = 0; i < level.walls.size(); ++i) {
            if (level.walls[i] != other.level.walls[i]) {
                return false;
            }
        }

        for (const auto &goalPair : level.goalsMap) {
            if (goalPair.second != other.level.goalsMap.at(goalPair.first)) {
                return false;
            }
        }

        return true;
    }

    // @todo: This is not a good comparison operator.
    bool operator<(const State &other) const { return this->getG() < other.getG(); }

    bool isApplicable(const Agent &agent, const Action &action) const {
        Point2D agentDestination, boxPosition;
        char boxIdConsidered;

        switch (action.type) {
            case ActionType::NoOp:
                return true;

            case ActionType::Move:
                agentDestination = agent.position() + action.agentDelta;
                return cellIsFree(agentDestination);

            case ActionType::Push: {
                boxPosition = agent.position() + action.agentDelta;
                boxIdConsidered = boxIdAt(boxPosition);
                if (boxIdConsidered == 0) return false;

                const Box &box = level.boxesMap.at(boxIdConsidered);
                return agent.color() == box.color() && cellIsFree(boxPosition);
            }

            case ActionType::Pull: {
                boxPosition = agent.position() - action.agentDelta;
                boxIdConsidered = boxIdAt(boxPosition);
                if (boxIdConsidered == 0) return false;

                const Box &box = level.boxesMap.at(boxIdConsidered);
                agentDestination = agent.position() + action.agentDelta;
                return agent.color() == box.color() && cellIsFree(agentDestination);
            }
            default:
                throw std::invalid_argument("Invalid action type");
        }
    }

    bool isConflicting(const std::vector<const Action *> &jointAction) const {
        size_t numAgents = level.agentsMap.size();
        std::vector<Point2D> destinations(numAgents);
        // std::vector<Point2D> boxPositions(numAgents);
        Point2D destBoxPos;

        for (const auto &agentPair : level.agentsMap) {
            int agentIndex = agentPair.first - FIRST_AGENT;
            const Action *action = jointAction[agentIndex];
            const Agent &agent = agentPair.second;

            switch (action->type) {
                case ActionType::NoOp:
                    break;

                case ActionType::Move:
                    destinations[agentIndex] = agent.position() + action->agentDelta;
                    // boxPositions[agentIndex] = agent.position();  // @todo: Distinct dummy value
                    break;

                case ActionType::Push:
                    destBoxPos = agent.position() + action->agentDelta;
                    destinations[agentIndex] = destBoxPos + action->boxDelta;
                    // boxPositions[agentIndex] = destBoxPos;
                    break;

                case ActionType::Pull:
                    destBoxPos = agent.position() - action->boxDelta;
                    destinations[agentIndex] = destBoxPos + action->agentDelta;
                    // boxPositions[agentIndex] = destBoxPos;
                    break;

                default:
                    throw std::invalid_argument("Invalid action type");
                    // return false;
            }
        }

        for (size_t a1 = 0; a1 < numAgents; ++a1) {
            if (jointAction[a1]->type == ActionType::NoOp) continue;

            for (size_t a2 = a1 + 1; a2 < numAgents; ++a2) {
                if (jointAction[a2]->type == ActionType::NoOp) continue;

                if (destinations[a1] == destinations[a2]) {
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
                const char boxId = boxIdAt(Point2D(row, col));
                const char agentId = agentIdAt(Point2D(row, col));

                if (boxId != 0) {
                    ss << boxId;
                    continue;
                }

                if (agentId != 0) {
                    ss << agentId;
                    continue;
                }

                if (level.walls[row][col]) {
                    ss << WALL;
                    continue;
                }

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
            for (const auto &agentPair : s->level.agentsMap) {
                seed ^= std::hash<int>()(agentPair.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            // Combine hashes of agent cols
            for (const auto &agentPair : s->level.agentsMap) {
                seed ^= std::hash<int>()(agentPair.second.position().y()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            // Combine hashes of boxes (vector of vectors)
            for (const auto &boxPair : s->level.boxesMap) {
                seed ^= std::hash<char>()(boxPair.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };

   private:
    // Creates a new state from a parent state and a joint action performed in that state.
    State(const State *parent, std::vector<const Action *> jointAction)
        : level(parent->level), parent(parent), jointAction(jointAction), g_(parent->getG() + 1) {
        for (auto &agentPair : level.agentsMap) {
            const int agentIndex = agentPair.first - FIRST_AGENT;
            const Action *action = jointAction[agentIndex];
            Agent &agent = agentPair.second;

            switch (action->type) {
                case ActionType::NoOp:
                    break;

                case ActionType::Move:
                    agent.moveBy(action->agentDelta);
                    break;

                case ActionType::Push: {
                    agent.moveBy(action->agentDelta);
                    const Point2D boxPosition = agent.position();
                    const char boxId = boxIdAt(boxPosition);
                    if (boxId == 0) {
                        throw std::invalid_argument("Box not found");
                    }

                    Box &box = level.boxesMap.at(boxId);
                    box.moveBy(action->boxDelta);
                    break;
                }

                case ActionType::Pull: {
                    const Point2D boxPosition = agent.position() - action->agentDelta;
                    const char boxId = boxIdAt(boxPosition);
                    if (boxId == 0) {
                        throw std::invalid_argument("Box not found");
                    }

                    Box &box = level.boxesMap.at(boxId);
                    box.moveBy(action->boxDelta);
                    agent.moveBy(action->agentDelta);
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid action type");
            }
        }
    }

    const int g_;  // Cost of reaching this state

    // Returns true if the cell at the given position is free (i.e. not a wall, box, or agent)
    bool cellIsFree(const Point2D &position) const {
        if (level.walls[position.x()][position.y()]) return false;
        if (boxIdAt(position) != 0) return false;
        if (agentIdAt(position) != 0) return false;
        return true;
    }

    // Returns the id of the agent at the given position, or 0
    char agentIdAt(const Point2D &position) const {
        for (const auto &agentPair : level.agentsMap) {
            if (agentPair.second.position() == position) {
                return agentPair.first;
            }
        }
        return 0;
    }

    // Returns the id of the box at the given position, or 0
    char boxIdAt(const Point2D &position) const {
        for (const auto &boxPair : level.boxesMap) {
            if (boxPair.second.position() == position) {
                return boxPair.first;
            }
        }
        return 0;
    }
};

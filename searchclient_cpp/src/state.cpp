#include "state.hpp"

#include <random>

#include "helpers.hpp"

State::State(Level level) : level(level), parent(nullptr), g_(0) { jointAction.reserve(level.agentsMap.size()); }

std::vector<std::vector<const Action *>> State::extractPlan() const {
    std::vector<std::vector<const Action *>> plan;
    const State *current = this;

    while (current->parent != nullptr) {
        plan.push_back(current->jointAction);
        current = current->parent;
    }

    std::reverse(plan.begin(), plan.end());
    return plan;
}

std::vector<State *> State::getExpandedStates() const {
    std::vector<State *> expandedStates;

    size_t numAgents = level.agentsMap.size();
    std::vector<std::vector<const Action *>> applicableAction(numAgents);

    for (const Action *actionPtr : Action::allValues()) {
        for (const auto &agentPair : level.agentsMap) {
            if (!isApplicable(agentPair.second, *actionPtr)) {
                continue;
            }
            applicableAction[agentPair.first - FIRST_AGENT].push_back(actionPtr);
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
    // Improves dfs ~4 times, worsens bfs ~3 times
    // std::shuffle(expandedStates.begin(), expandedStates.end(), std::random_device());
    return expandedStates;
}

size_t State::getHash() const {
    if (hash_ != 0) {
        return hash_;
    }

    size_t seed = 0;
    for (const auto &agentPair : level.agentsMap) {
        utils::hashCombine(seed, agentPair.second.position());
    }
    for (const auto &boxPair : level.boxesMap) {
        utils::hashCombine(seed, boxPair.second.position());
    }

    hash_ = (seed == 0) ? 1 : seed;
    return hash_;
}

bool State::operator==(const State &other) const {
    if (this == &other) return true;

    if (level == other.level) {
        return false;
    }

    return true;
}

// @todo: This is not a good comparison operator.
bool State::operator<(const State &other) const { return this->getG() < other.getG(); }

bool State::isApplicable(const Agent &agent, const Action &action) const {
    Point2D agentDestination, boxPosition;
    char boxIdConsidered;

    switch (action.type) {
        case ActionType::NoOp:
            return true;

        case ActionType::Move:
            agentDestination = agent.position() + action.agentDelta;
            return isCellFree(agentDestination);

        case ActionType::Push: {
            boxPosition = agent.position() + action.agentDelta;
            boxIdConsidered = isCellBox(boxPosition) ? charAt(boxPosition) : EMPTY;
            if (boxIdConsidered == EMPTY) return false;

            const Box &box = level.boxesMap.at(boxIdConsidered);
            return agent.color() == box.color() && isCellFree(boxPosition);
        }

        case ActionType::Pull: {
            boxPosition = agent.position() - action.agentDelta;
            boxIdConsidered = isCellBox(boxPosition) ? charAt(boxPosition) : EMPTY;
            if (boxIdConsidered == EMPTY) return false;

            const Box &box = level.boxesMap.at(boxIdConsidered);
            agentDestination = agent.position() + action.agentDelta;
            return agent.color() == box.color() && isCellFree(agentDestination);
        }
        default:
            throw std::invalid_argument("Invalid action type");
    }
}

bool State::isGoalState() const {
    for (const auto &goal : level.goalsMap) {
        const char goalId = goal.first;
        const Point2D goalPos = goal.second.position();

        // Every box must be on a goal
        for (const auto &boxPair : level.boxesMap) {
            if (boxPair.first == goalId && !boxPair.second.at(goalPos)) return false;
        }

        // Every agent must be on a goal
        for (const auto &agentPair : level.agentsMap) {
            if (agentPair.first == goalId && !agentPair.second.at(goalPos)) return false;
        }
    }
    return true;
}

bool State::isConflicting(const std::vector<const Action *> &jointAction) const {
    size_t numAgents = level.agentsMap.size();
    std::vector<Point2D> destinations(numAgents);
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
                break;

            case ActionType::Push:
                destBoxPos = agent.position() + action->agentDelta;
                destinations[agentIndex] = destBoxPos + action->boxDelta;
                break;

            case ActionType::Pull:
                destBoxPos = agent.position() - action->boxDelta;
                destinations[agentIndex] = destBoxPos + action->agentDelta;
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

std::string State::toString() const {
    std::stringstream ss;
    ss << "State(agentRows=\n";
    for (size_t row = 0; row < level.walls.size(); row++) {
        const size_t col_size = level.walls[row].size();
        for (size_t col = 0; col < col_size; col++) {
            const char boxId = charAt(Point2D(row, col));
            const char agentId = charAt(Point2D(row, col));

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

State::State(const State *parent, std::vector<const Action *> jointAction)
    : level(parent->level), parent(parent), jointAction(jointAction), g_(parent->getG() + 1) {
    for (auto &agentPair : level.agentsMap) {
        const int agentIndex = agentPair.first - FIRST_AGENT;
        const Action *action = jointAction[agentIndex];
        Agent &agent = agentPair.second;

        switch (action->type) {
            case ActionType::NoOp:
                break;

            case ActionType::Move:
                level.moveAgent(agentPair.first, action);
                break;

            case ActionType::Push: {
                level.moveAgent(agentPair.first, action);
                const Point2D boxPosition = agent.position();
                if (!level.isCellBox(boxPosition)) {
                    return;
                }

                level.moveBox(level.charAt(boxPosition), action);
                break;
            }

            case ActionType::Pull: {
                const Point2D boxPosition = agent.position();
                level.moveAgent(agentPair.first, action);
                if (!level.isCellBox(boxPosition)) {
                    return;
                }

                level.moveBox(level.charAt(boxPosition), action);
                break;
            }

            default:
                throw std::invalid_argument("Invalid action type");
        }
    }
}

bool State::isCellFree(const Point2D &position) const { return level.isCellEmpty(position); }

bool State::isCellBox(const Point2D &position) const { return level.isCellBox(position); }

bool State::isCellAgent(const Point2D &position) const { return level.isCellAgent(position); }

char State::charAt(const Point2D &position) const { return level.charAt(position); }

#include "state.hpp"

#include <random>

#include "action.hpp"
#include "feature_flags.hpp"
#include "helpers.hpp"

std::random_device State::rd_;
//std::mt19937 State::g_rd_(rd_());
std::mt19937 State::g_rd_(1234); // make it deterministic

State::State(Level level) : level(level), parent(nullptr), g_(0) {}

[[nodiscard]] void *State::operator new(std::size_t size) {
#ifdef USE_STATE_MEMORY_POOL
    return StateMemoryPoolAllocator::getInstance().allocate(size);
#else
    return ::operator new(size);
#endif
}

void State::operator delete(void *ptr) noexcept {
#ifdef USE_STATE_MEMORY_POOL
    StateMemoryPoolAllocator::getInstance().deallocate(ptr);
#else
    return ::operator delete(ptr);
#endif
}

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

    //size_t numAgents = level.agentsMap.size();
    std::vector<std::vector<const Action *>> applicableAction(level.agents.size());

    for (const Action *actionPtr : Action::allValues()) {
        //for (const auto &agentPair : level.agentsMap) {
        for (size_t agent_idx = 0; agent_idx < level.agents.size(); agent_idx++) {
            if (!isApplicable(agent_idx, *actionPtr)) {
                continue;
            }
            applicableAction[agent_idx].push_back(actionPtr);
        }
    }

    std::vector<size_t> actionsPermutation(level.agents.size(), 0);

    while (true) {
        std::vector<const Action *> currentJointAction;
        for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
            currentJointAction.push_back(applicableAction[agent_idx][actionsPermutation[agent_idx]]);
        }

        if (!isConflicting(currentJointAction)) {
            //if (true && currentJointAction[0]->type == ActionType::Move && currentJointAction[1]->type == ActionType::Move && g_ == 4
            //    && currentJointAction[0]->agentDelta == Point2D(1, 0)
            //    && currentJointAction[1]->agentDelta == Point2D(0, 1)) 
            //{
            //    fprintf(stderr, "Pull and Push: %s\n", toString().c_str());
            //    fprintf(stderr, "Plan: %s\n", formatJointAction(currentJointAction, false).c_str());
            //    fprintf(stderr, "currentJointAction[0]->boxDelta: %d %d\n", currentJointAction[0]->boxDelta.x(), currentJointAction[0]->boxDelta.y());
            //    fprintf(stderr, "currentJointAction[0]->agentDelta: %d %d\n", currentJointAction[0]->agentDelta.x(), currentJointAction[0]->agentDelta.y());
            //    isConflicting(currentJointAction, true);
            //}

            expandedStates.push_back(new State(this, currentJointAction));
        }

        bool done = false;
        for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
            if (actionsPermutation[agent_idx] < applicableAction[agent_idx].size() - 1) {
                actionsPermutation[agent_idx]++;
                break;
            }
            actionsPermutation[agent_idx] = 0;
            if (agent_idx == level.agents.size() - 1) done = true;
        }
        if (done) break;
    }

    // Improves dfs ~4 times, worsens bfs ~3 times
#ifdef USE_STATE_SHUFFLE
    std::shuffle(expandedStates.begin(), expandedStates.end(), State::g_rd_);
#endif
    return expandedStates;
}

// TODO: this is extremely slow, we should use a better hash function that is only computed once after state modification/creation and then reused
size_t State::getHash() const {
    /*
    if (hash_ != 0) {
        return hash_;
    }

    size_t seed = 0x1u;
    for (const auto &agentPair : level.agentsMap) {
        utils::hashCombine(seed, agentPair.second.position());
    }
    for (const auto &boxPair : level.boxesMap) {
        utils::hashCombine(seed, boxPair.second.position());
    }
    hash_ = (seed == 0) ? 1 : seed;
    return hash_;
    */

    // Compute efficiently hash of agent vector
    auto byte_ptr = reinterpret_cast<const char*>(level.agents.data());
    auto byte_len = level.agents.size() * sizeof(Cell2D);
    // std::hash<string_view> typically is FNV-1a or similar
    size_t agents_hash = std::hash<std::string_view>()(
        std::string_view{ byte_ptr, byte_len }
    );

    size_t boxes_hash = level.boxes.get_hash();

    return agents_hash ^ boxes_hash;
}

bool State::operator==(const State &other) const {
    /*
    if (this == &other) return true;

    if (level.agentsMap.size() != other.level.agentsMap.size() || level.boxesMap.size() != other.level.boxesMap.size()) {
        return false;
    }

    for (const auto &agentPair : level.agentsMap) {
        if (agentPair.second != other.level.agentsMap.at(agentPair.first)) {
            return false;
        }
    }

    for (const auto &boxPair : level.boxesMap) {
        if (boxPair.second != other.level.boxesMap.at(boxPair.first)) {
            return false;
        }
    }

    return true;
    */

    return level == other.level;
}

bool State::isApplicable(int agent_idx, const Action &action) const {
    /*
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
            agentDestination = box.position() + action.boxDelta;
            return agent.color() == box.color() && cellIsFree(agentDestination);
        }

        case ActionType::Pull: {
            boxPosition = agent.position() - action.boxDelta;
            boxIdConsidered = boxIdAt(boxPosition);
            if (boxIdConsidered == 0) return false;

            const Box &box = level.boxesMap.at(boxIdConsidered);
            agentDestination = agent.position() + action.agentDelta;
            return agent.color() == box.color() && cellIsFree(agentDestination);
        }
        default:
            fprintf(stderr, "Invalid action type: %d\n", action.type);
            throw std::invalid_argument("Invalid action type");
    }
    */
    

    switch (action.type) {
        case ActionType::NoOp:
            return true;
        
        case ActionType::Move:
        {
            Cell2D destination = level.agents[agent_idx] + action.agent_delta;
            return is_cell_free(destination);
        }

        case ActionType::Push:
        {
            // Box is now where the agent will be in a moment
            Cell2D box_position = level.agents[agent_idx] + action.agent_delta;
            Cell2D destination = box_position + action.box_delta;
            char box_id = level.boxes(box_position);
            return is_cell_free(destination) && box_id && level.agent_colors[agent_idx] == level.box_colors[box_id - 'A'];
        }
            
        case ActionType::Pull:
        {
            // Box will be in a moment where the agent is now
            Cell2D box_position = level.agents[agent_idx] - action.box_delta;
            Cell2D destination = level.agents[agent_idx] + action.agent_delta;
            char box_id = level.boxes(box_position);
            return is_cell_free(destination) && box_id && level.agent_colors[agent_idx] == level.box_colors[box_id - 'A'];
        }
            
        default:
            throw std::invalid_argument("Invalid action type");
   }

    // Unreachable:
    return false;
}

bool State::isGoalState() const {
    /*
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
    */

    // Check first if all agents are on goals (as this is very likely to fail and cut the computation short)
    for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
        if (level.goals(level.agents[agent_idx]) != agent_idx + '0') {
            return false;
        }
    }

    for (size_t i = 0; i < level.goals.data.size(); ++i) {
        if (FIRST_BOX <= level.goals.data[i] && level.goals.data[i] <= LAST_BOX && level.goals.data[i] != level.boxes.data[i]) {
            return false;
        }
    }

    return true;
}

bool State::isConflicting(const std::vector<const Action *> &jointAction, const bool debug) const {
    //size_t numAgents = level.agentsMap.size();
    //std::vector<Point2D> destinations(numAgents);
    // std::vector<Point2D> boxPositions(numAgents);
    //Point2D destBoxPos;

    /*
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
                //destBoxPos = agent.position() - action->boxDelta;
                destinations[agentIndex] = agent.position() + action->agentDelta;
                // boxPositions[agentIndex] = destBoxPos;
                break;

            default:
                throw std::invalid_argument("Invalid action type");
                // return false;
        }
    }
    */

    std::vector<Cell2D> destinations(level.agents.size());

    for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
        switch (jointAction[agent_idx]->type) {
            case ActionType::NoOp:
                destinations[agent_idx] = level.agents[agent_idx];
                break;

            case ActionType::Move:
                destinations[agent_idx] = level.agents[agent_idx] + jointAction[agent_idx]->agent_delta;
                break;

            case ActionType::Push:
            {
                Cell2D box_position = level.agents[agent_idx] + jointAction[agent_idx]->agent_delta;
                destinations[agent_idx] = box_position + jointAction[agent_idx]->box_delta;
                break;
            }

            case ActionType::Pull:
            {
                Cell2D box_position = level.agents[agent_idx] - jointAction[agent_idx]->box_delta;
                destinations[agent_idx] = level.agents[agent_idx] + jointAction[agent_idx]->agent_delta;
                break;
            }
        }
    }

    if (debug) {
        for (size_t agent = 0; agent < level.agents.size(); ++agent) {
            fprintf(stderr, "agent: %d %d %d\n", agent, destinations[agent].r, destinations[agent].c);
        }
    }

    for (size_t a1 = 0; a1 < level.agents.size(); ++a1) {
        if (jointAction[a1]->type == ActionType::NoOp) continue;

        for (size_t a2 = a1 + 1; a2 < level.agents.size(); ++a2) {
            if (jointAction[a2]->type == ActionType::NoOp) continue;

            if (destinations[a1] == destinations[a2]) {
                return true;
            }
        }
    }
    return false;
}

std::string State::toString() const {
    CharGrid grid = level.walls;
    for (size_t i = 0; i < level.boxes.data.size(); ++i) {
        grid.data[i] &= level.boxes.data[i];
    }

    for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
        grid(level.agents[agent_idx]) = agent_idx + '0';
    }

    return grid.to_string();

    /*
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
    */
}

State::State(const State *parent, std::vector<const Action *> jointAction)
    : level(parent->level), parent(parent), jointAction(jointAction), g_(parent->getG() + 1) {
    /*
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
                const Point2D boxPosition = agent.position() - action->boxDelta;
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
    */

    for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
        switch (jointAction[agent_idx]->type) {
            case ActionType::NoOp:
                break;

            case ActionType::Move:
                level.agents[agent_idx] += jointAction[agent_idx]->agent_delta;
                break;

            case ActionType::Push:
            {
                level.agents[agent_idx] += jointAction[agent_idx]->agent_delta;
                Cell2D box_position = level.agents[agent_idx] + jointAction[agent_idx]->agent_delta;
                level.boxes(box_position + jointAction[agent_idx]->box_delta) = level.boxes(box_position);
                level.boxes(box_position) = 0;
                break;
            }

            case ActionType::Pull:
            {
                level.agents[agent_idx] += jointAction[agent_idx]->agent_delta;
                Cell2D box_position = level.agents[agent_idx] - jointAction[agent_idx]->box_delta;
                level.boxes(box_position + jointAction[agent_idx]->box_delta) = level.boxes(box_position);
                level.boxes(box_position) = 0;
                break;
            }

            default:
                throw std::invalid_argument("Invalid action type");
        }
    }
}

/*
bool State::cellIsFree(const Point2D &position) const {
    if (level.walls[position.x()][position.y()]) return false;
    if (boxIdAt(position) != 0) return false;
    if (agentIdAt(position) != 0) return false;
    return true;
}
*/

bool State::is_cell_free(const Cell2D &cell) const {
    if (level.walls(cell) || level.boxes(cell))
        return false;

    for (const auto &agent_position : level.agents) {
        if (agent_position == cell)
            return false;
    }

    return true;
}

/*
char State::agentIdAt(const Point2D &position) const {
    for (const auto &agentPair : level.agentsMap) {
        if (agentPair.second.position() == position) {
            return agentPair.first;
        }
    }
    return 0;
}

char State::boxIdAt(const Point2D &position) const {
    for (const auto &boxPair : level.boxesMap) {
        if (boxPair.second.position() == position) {
            return boxPair.first;
        }
    }
    return 0;
}
*/

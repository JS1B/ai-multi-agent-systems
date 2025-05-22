#include "state.hpp"

#include <random>

#include "action.hpp"
#include "feature_flags.hpp"
#include "helpers.hpp"

std::random_device State::rd_;
//std::mt19937 State::g_rd_(rd_());
std::mt19937 State::g_rd_(1234); // make it deterministic

State::State(Level level) : level(level), parent(nullptr), g_(0) {
    hash_ = getHash();
}

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
            expandedStates.push_back(new State(this, currentJointAction)); // TODO: optimize allocation by providing a preallocated vector
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

size_t State::getHash() const {
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
    //return level == other.level;
    return level.agents == other.level.agents && level.boxes == other.level.boxes; // this should be faster
}

bool State::isApplicable(int agent_idx, const Action &action) const {
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
   // Check first if all agents are on goals (as this is very likely to fail and cut the computation short)
    for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
        if (level.agent_goals[agent_idx].r != 0 && level.agent_goals[agent_idx] != level.agents[agent_idx]) {
            return false;
        }
    }

    for (size_t i = 0; i < level.box_goals.data.size(); ++i) {
        // We do not need to check if the char is a letter, as it is guaranteed to be a box
        if (level.box_goals.data[i] && level.box_goals.data[i] != level.boxes.data[i]) {
            return false;
        }
    }

    return true;
}

bool State::isConflicting(const std::vector<const Action *> &jointAction, const bool debug) const {
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
                //Cell2D box_position = level.agents[agent_idx] - jointAction[agent_idx]->box_delta;
                destinations[agent_idx] = level.agents[agent_idx] + jointAction[agent_idx]->agent_delta;
                break;
            }
        }
    }

    if (debug) {
        for (size_t agent = 0; agent < level.agents.size(); ++agent) {
            fprintf(stderr, "agent: %zu %d %d\n", agent, destinations[agent].r, destinations[agent].c);
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
        grid.data[i] |= level.boxes.data[i];
    }

    for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
        grid(level.agents[agent_idx]) = agent_idx + '0';
    }

    return grid.to_string();
}

State::State(const State *parent, std::vector<const Action *> jointAction)
    : level(parent->level), parent(parent), jointAction(jointAction), g_(parent->getG() + 1) {

    for (size_t agent_idx = 0; agent_idx < level.agents.size(); ++agent_idx) {
        switch (jointAction[agent_idx]->type) {
            case ActionType::NoOp:
                break;

            case ActionType::Move:
                level.agents[agent_idx] += jointAction[agent_idx]->agent_delta;
                break;

            case ActionType::Push:
            {
                Cell2D box_position = level.agents[agent_idx] + jointAction[agent_idx]->agent_delta;
                level.agents[agent_idx] += jointAction[agent_idx]->agent_delta;
                level.boxes(box_position + jointAction[agent_idx]->box_delta) = level.boxes(box_position);
                level.boxes(box_position) = 0;
                break;
            }

            case ActionType::Pull:
            {
                Cell2D box_position = level.agents[agent_idx] - jointAction[agent_idx]->box_delta;
                level.agents[agent_idx] += jointAction[agent_idx]->agent_delta;
                level.boxes(box_position + jointAction[agent_idx]->box_delta) = level.boxes(box_position);
                level.boxes(box_position) = 0;
                break;
            }

            default:
                throw std::invalid_argument("Invalid action type");
        }
    }

    hash_ = getHash();
}

bool State::is_cell_free(const Cell2D &cell) const {
    if (level.walls(cell) || level.boxes(cell))
        return false;

    for (const auto &agent_position : level.agents) {
        if (agent_position == cell)
            return false;
    }

    return true;
}

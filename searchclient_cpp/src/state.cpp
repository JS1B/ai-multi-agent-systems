#include "state.hpp"

#include <random>

#include "feature_flags.hpp"
#include "helpers.hpp"

std::random_device State::rd_;
std::mt19937 State::g_rd_(rd_());

State::State(const Level& level_ref) 
    : level_(level_ref), 
      currentAgents_(level_ref.agentsMap), 
      currentBoxes_(level_ref.boxesMap), 
      parent(nullptr), 
      g_(0) {
    // Initial hash can be calculated here if desired, or lazily as before.
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

    // Use numAgents() which now correctly refers to currentAgents_.size()
    size_t num_agents = this->numAgents(); 
    if (num_agents == 0) {
        return expandedStates;
    }

    std::vector<std::vector<const Action *>> applicableActionPerAgent(num_agents);
    std::vector<char> agent_ids_in_order; // To map agent_idx back to agent_id if needed, and for consistent iteration
    agent_ids_in_order.reserve(num_agents);
    for(const auto& agentPair : this->currentAgents_) { // Iterate current agents
        agent_ids_in_order.push_back(agentPair.first);
    }
    // Sort agent IDs to ensure consistent order for applicableActionPerAgent indexing
    // and for the permutation logic, assuming jointAction is indexed based on this sorted order.
    std::sort(agent_ids_in_order.begin(), agent_ids_in_order.end());

    for (size_t agent_idx = 0; agent_idx < num_agents; ++agent_idx) {
        char agent_id = agent_ids_in_order[agent_idx];
        const Agent& agent = this->currentAgents_.at(agent_id); // Get agent from current state
        
        for (const Action *actionPtr : Action::allValues()) {
            if (isApplicable(agent, *actionPtr)) { // Pass agent from current state
                applicableActionPerAgent[agent_idx].push_back(actionPtr);
            }
        }
        if (applicableActionPerAgent[agent_idx].empty()) {
            // This should ideally not happen if NoOp is always applicable and agents exist.
            // If it does, it means an agent has no possible action.
        }
    }

    std::vector<size_t> actionsPermutation(num_agents, 0);

    while (true) {
        std::vector<const Action *> currentJointAction(num_agents);
        bool possible_to_form_joint_action = true;
        for (size_t agent_idx = 0; agent_idx < num_agents; ++agent_idx) {
            if (applicableActionPerAgent[agent_idx].empty()) {
                possible_to_form_joint_action = false;
                break;
            }
            currentJointAction[agent_idx] = applicableActionPerAgent[agent_idx][actionsPermutation[agent_idx]];
        }

        if (!possible_to_form_joint_action) {
            break; 
        }
        
        if (!isConflicting(currentJointAction)) { 
            // Pass this->level_ to the State constructor
            expandedStates.push_back(new State(this, currentJointAction, this->level_));
        } 

        bool done = false;
        for (size_t agent_idx = 0; agent_idx < num_agents; ++agent_idx) {
            if (applicableActionPerAgent[agent_idx].empty()) { 
                done = true; 
                break;
            }
            if (actionsPermutation[agent_idx] < applicableActionPerAgent[agent_idx].size() - 1) {
                actionsPermutation[agent_idx]++;
                break; 
            }
            actionsPermutation[agent_idx] = 0; 
            if (agent_idx == num_agents - 1) done = true; 
        }
        if (done) break; 
    }

#ifdef USE_STATE_SHUFFLE
    std::shuffle(expandedStates.begin(), expandedStates.end(), State::g_rd_);
#endif
    return expandedStates;
}

size_t State::getHash() const {
    if (hash_ != 0) {
        return hash_;
    }

    size_t seed = 0x1u;
    // Hash based on current agent positions
    std::vector<char> agent_keys;
    for(const auto& pair : this->currentAgents_) agent_keys.push_back(pair.first);
    std::sort(agent_keys.begin(), agent_keys.end()); // Sort keys for consistent hash order
    for(char key : agent_keys){
        utils::hashCombine(seed, this->currentAgents_.at(key).position());
        // utils::hashCombine(seed, this->currentAgents_.at(key).getId()); // Optionally include ID if colors can change etc.
    }

    // Hash based on current box positions
    std::vector<char> box_keys;
    for(const auto& pair : this->currentBoxes_) box_keys.push_back(pair.first);
    std::sort(box_keys.begin(), box_keys.end()); // Sort keys for consistent hash order
    for(char key : box_keys){
        utils::hashCombine(seed, this->currentBoxes_.at(key).position());
        // utils::hashCombine(seed, this->currentBoxes_.at(key).getId()); // Optionally include ID
        // utils::hashCombine(seed, static_cast<int>(this->currentBoxes_.at(key).color())); // Optionally include color
    }
    
    hash_ = (seed == 0) ? 1 : seed; // Ensure hash is never 0 if 0 is used as uncomputed flag
    return hash_;
}

bool State::operator==(const State &other) const {
    if (this == &other) return true;

    // Compare current agent maps
    if (this->currentAgents_.size() != other.currentAgents_.size()) return false;
    for (const auto &agentPair : this->currentAgents_) {
        auto other_it = other.currentAgents_.find(agentPair.first);
        if (other_it == other.currentAgents_.end() || agentPair.second != other_it->second) {
            return false;
        }
    }

    // Compare current box maps
    if (this->currentBoxes_.size() != other.currentBoxes_.size()) return false;
    for (const auto &boxPair : this->currentBoxes_) {
        auto other_it = other.currentBoxes_.find(boxPair.first);
        if (other_it == other.currentBoxes_.end() || boxPair.second != other_it->second) {
            return false;
        }
    }
    
    // Level reference should ideally point to the same static level object if properly managed.
    // If Level objects can be different but semantically identical, you might need a Level::operator==
    // For now, we assume level_ comparison isn't the primary factor for state equality if agents/boxes match.
    // if (&this->level_ != &other.level_) { /* Potentially log or handle if different level instances are used */ }

    return true;
}

bool State::isApplicable(const Agent &agent_from_current_state, const Action &action) const {
    Point2D agentPos = agent_from_current_state.position();
    Color agentColor = agent_from_current_state.color();

    switch (action.type) {
        case ActionType::NoOp:
            return true;

        case ActionType::Move: {
            Point2D newAgentPos = agentPos + action.agentDelta;
            return cellIsFree(newAgentPos);
        }
        case ActionType::Push: {
            Point2D boxPos = agentPos + action.agentDelta; // Expected position of the box
            char boxId = this->boxIdAt(boxPos); // Check our current state for a box here
            if (boxId == 0) return false; // No box to push in current state

            // Get the actual box from our current state to check its color
            const Box& actualBox = this->currentBoxes_.at(boxId);

            if (actualBox.color() != agentColor) return false; // Agent cannot push box of different color

            Point2D newBoxPos = boxPos + action.boxDelta; // Box's new position
            return cellIsFree(newBoxPos);
        }
        case ActionType::Pull: {
            Point2D newAgentPos = agentPos + action.agentDelta;
            Point2D boxOriginalPos = agentPos - action.boxDelta; // Original position of the box relative to current agent pos
            char boxId = this->boxIdAt(boxOriginalPos); // Check our current state for a box here
            if (boxId == 0) return false; // No box to pull

            const Box& actualBox = this->currentBoxes_.at(boxId);

            if (actualBox.color() != agentColor) return false; // Agent cannot pull box of different color
            
            return cellIsFree(newAgentPos); // Agent's new position must be free
        }
    }
    return false; 
}

bool State::isGoalState() const {
    // Check if all agents in the current state are at their goals defined in the level
    for (const auto &agentPair : this->currentAgents_) {
        const Agent &agent = agentPair.second;
        Point2D agent_pos = agent.position();

        // Goal definitions are in the static level_ data
        auto goal_it = this->level_.goalsMap_.find(agent.getId()); 
        if (goal_it != this->level_.goalsMap_.end()) { 
            const Goal &agent_goal = goal_it->second;
            if (agent_pos != agent_goal.position()) {
                return false; 
            }
        }
    }

    // Check if all boxes in the current state are at their goals defined in the level
    for (const auto &boxPair : this->currentBoxes_) {
        const Box &box = boxPair.second;
        Point2D box_pos = box.position();

        auto goal_it = this->level_.goalsMap_.find(box.getId()); 
        if (goal_it != this->level_.goalsMap_.end()) { 
            const Goal &box_goal = goal_it->second;
            if (box_pos != box_goal.position()) {
                return false; 
            }
        } 
    }
    return true;
}

bool State::isConflicting(const std::vector<const Action *> &jointAction) const {
    // numAgents() now refers to currentAgents_.size()
    size_t num_agents = this->numAgents(); 
    if (jointAction.size() != num_agents) {
        // This would be an internal error, jointAction should match number of agents
        return true; // Treat as conflicting if sizes don't match
    }

    std::vector<Point2D> agent_final_positions(num_agents);
    std::vector<Point2D> box_final_positions;
    std::vector<char> moved_box_ids; 

    // Get agent IDs in a consistent order to map jointAction indices correctly
    std::vector<char> agent_ids_in_order;
    agent_ids_in_order.reserve(num_agents);
    for(const auto& agentPair : this->currentAgents_) { // Iterate current agents
        agent_ids_in_order.push_back(agentPair.first);
    }
    std::sort(agent_ids_in_order.begin(), agent_ids_in_order.end());

    for (size_t agent_idx = 0; agent_idx < num_agents; ++agent_idx) {
        char agent_id = agent_ids_in_order[agent_idx];
        const Agent &agent = this->currentAgents_.at(agent_id); // Agent from current state
        const Action *action = jointAction[agent_idx]; // Action for this agent_idx

        switch (action->type) {
            case ActionType::NoOp:
                agent_final_positions[agent_idx] = agent.position();
                break;

            case ActionType::Move:
                agent_final_positions[agent_idx] = agent.position() + action->agentDelta;
                break;

            case ActionType::Push: {
                Point2D box_original_pos = agent.position() + action->agentDelta;
                agent_final_positions[agent_idx] = box_original_pos; 
                
                // We need the ID of the box at box_original_pos FROM THE CURRENT STATE
                char current_box_id_at_pos = this->boxIdAt(box_original_pos);
                if (current_box_id_at_pos != 0) { 
                    box_final_positions.push_back(box_original_pos + action->boxDelta);
                    moved_box_ids.push_back(current_box_id_at_pos);
                }
                break;
            }

            case ActionType::Pull: {
                agent_final_positions[agent_idx] = agent.position() + action->agentDelta; 
                Point2D box_original_pos = agent.position() - action->boxDelta; 
                
                char current_box_id_at_pos = this->boxIdAt(box_original_pos);
                if (current_box_id_at_pos != 0) { 
                    box_final_positions.push_back(agent.position()); // Box moves to where the agent *was*
                    moved_box_ids.push_back(current_box_id_at_pos);
                }
                break;
            }
            default:
                throw std::invalid_argument("Invalid action type in isConflicting");
        }
    }

    // Check for agent-agent conflicts (two agents ending in the same cell)
    for (size_t a1 = 0; a1 < num_agents; ++a1) {
        for (size_t a2 = a1 + 1; a2 < num_agents; ++a2) {
            if (agent_final_positions[a1] == agent_final_positions[a2]) {
                return true; // Conflict: two agents at the same destination
            }
        }
    }

    // Check for box-box conflicts (two boxes ending in the same cell)
    for (size_t b1 = 0; b1 < box_final_positions.size(); ++b1) {
        for (size_t b2 = b1 + 1; b2 < box_final_positions.size(); ++b2) {
            if (box_final_positions[b1] == box_final_positions[b2]) {
                return true; // Conflict: two boxes at the same destination
            }
        }
    }

    // Check for agent-box conflicts (agent ends where a box also ends)
    for (size_t a = 0; a < num_agents; ++a) {
        for (size_t b = 0; b < box_final_positions.size(); ++b) {
            if (agent_final_positions[a] == box_final_positions[b]) {
                return true; // Conflict: agent and box at the same destination
            }
        }
    }
    
    // Add additional checks for swapping cells if necessary
    // e.g., agent1 moves A->B, agent2 moves B->A simultaneously.
    // This is not caught by simple destination checks if agentDelta implies a path.
    // However, our actions are atomic movements to cells, so this might be less of an issue
    // unless intermediate pathing is considered part of the conflict.
    // The current setup assumes actions are instantaneous transitions to final cells.

    return false; // No conflicts found
}

std::string State::toString() const {
    std::stringstream ss;
    ss << "State (g=" << g_ << ", parent_hash=" << (parent ? std::to_string(parent->getHash()) : "null") << "):\n";

    // Static level information for dimensions and walls
    int num_rows = this->level_.getRows();
    int num_cols = this->level_.getCols();

    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            Point2D current_pos(c, r); // x=col, y=row
            char char_to_print = EMPTY;

            if (this->level_.isWall(r,c)) { // Use level_ for wall info
                char_to_print = WALL;
            } else {
                // Dynamic info: agent and box positions from current state
                char agent_id = this->agentIdAt(current_pos); // Uses currentAgents_
                if (agent_id != 0) {
                    char_to_print = agent_id;
                } else {
                    char box_id = this->boxIdAt(current_pos); // Uses currentBoxes_
                    if (box_id != 0) {
                        char_to_print = box_id;
                    } else {
                        // Static info: goal locations from level_
                        bool is_goal = false;
                        for(const auto& goalPair : this->level_.goalsMap_){ // Use level_ for goal locations
                            if(goalPair.second.position() == current_pos){
                                char_to_print = tolower(goalPair.first); 
                                is_goal = true;
                                break;
                            }
                        }
                        if(!is_goal) char_to_print = EMPTY;
                    }
                }
            }
            ss << char_to_print;
        }
        ss << "\n";
    }
    return ss.str();
}

State::State(const State *parent_ptr, std::vector<const Action *> jointAction_param, const Level& level_ref)
    : level_(level_ref), // Or parent_ptr->level_ if level_ref is always the same static level
      currentAgents_(parent_ptr->currentAgents_), // Copy agents from parent
      currentBoxes_(parent_ptr->currentBoxes_),   // Copy boxes from parent
      parent(parent_ptr), 
      jointAction(jointAction_param), 
      g_(parent_ptr->getG() + 1) {

    // Now, apply jointAction to currentAgents_ and currentBoxes_
    // This loop needs to iterate based on agent IDs present in jointAction_param or currentAgents_
    // and then fetch mutable references from this->currentAgents_ and this->currentBoxes_.

    // Assuming jointAction_param is ordered by agent ID (0 to N-1)
    // or we get agent IDs from currentAgents_ keys.
    // For simplicity, let's iterate through currentAgents_ and use the agentIndex from its ID.
    // This assumes agents in currentAgents_ are the ones acting.

    for (auto& agentPair : this->currentAgents_) { // Iterate our own mutable map
        char agent_id = agentPair.first;
        Agent& agent = agentPair.second; // Get a mutable reference
        
        int agentIndex = agent_id - FIRST_AGENT; // Calculate index for jointAction_param
        if (agentIndex < 0 || static_cast<size_t>(agentIndex) >= jointAction_param.size()) {
            // This agent_id might not have an action in jointAction_param if sizes differ
            // Or, jointAction_param might be indexed differently. This needs robust handling.
            // For now, assume jointAction_param is correctly sized and ordered for agents in currentAgents_.
            // A safer way: iterate agent IDs from jointAction_param, ensure they exist in currentAgents_
            fprintf(stderr, "Warning: Agent ID %c (index %d) out of bounds for jointAction_param or logic error.\n", agent_id, agentIndex);
            continue; 
        }
        const Action *action = jointAction_param[agentIndex];

        switch (action->type) {
            case ActionType::NoOp:
                break;

            case ActionType::Move:
                agent.moveBy(action->agentDelta); // Modify our own agent copy
                break;

            case ActionType::Push: {
                Point2D box_original_pos = agent.position() + action->agentDelta;
                agent.moveBy(action->agentDelta); // Agent moves to where the box was
                
                // Find the box in *our* currentBoxes_ that was at box_original_pos
                // This requires a reverse lookup or knowing the box ID directly.
                // For now, let's assume action contains box_id or we can find it.
                // This is a critical part: how to identify WHICH box to move from currentBoxes_.
                // Let's iterate currentBoxes_ to find the one at box_original_pos.
                char box_to_move_id = 0;
                for(const auto& box_pair_check : this->currentBoxes_){
                    if(box_pair_check.second.position() == box_original_pos){
                        box_to_move_id = box_pair_check.first;
                        break;
                    }
                }

                if (box_to_move_id != 0) {
                    Box& box = this->currentBoxes_.at(box_to_move_id); // Get mutable ref
                    box.moveBy(action->boxDelta);    // Modify our own box copy
                } else {
                    // This implies an invalid Push action if isApplicable was correct
                    // or the box was moved by another agent already (not handled here yet for multi-agent push on same box)
                    // fprintf(stderr, "ERROR in State constructor: Push action by %c, but no box found at expected pre-push pos.\n", agent_id);
                    // For now, we might throw or log, as state might be inconsistent.
                    // throw std::runtime_error("Push: Box not found at expected pre-push position in current state.");
                }
                break;
            }

            case ActionType::Pull: {
                Point2D box_original_pos = agent.position() - action->boxDelta; // Box was "behind" agent's start
                // Agent moves first
                Point2D agent_start_pos = agent.position(); // Remember agent's start before moving agent
                agent.moveBy(action->agentDelta); // Modify our own agent copy

                // Find which box to move from currentBoxes_
                char box_to_move_id = 0;
                for(const auto& box_pair_check : this->currentBoxes_){
                    if(box_pair_check.second.position() == box_original_pos){
                        box_to_move_id = box_pair_check.first;
                        break;
                    }
                }

                if (box_to_move_id != 0) {
                    Box& box = this->currentBoxes_.at(box_to_move_id); // Get mutable ref
                    // Box moves to where the agent *was* before the agent moved
                    Point2D delta_for_box_to_agent_start = agent_start_pos - box.position();
                    box.moveBy(delta_for_box_to_agent_start); 
                } else {
                    // fprintf(stderr, "ERROR in State constructor: Pull action by %c, but no box found at expected pre-pull pos.\n", agent_id);
                    // throw std::runtime_error("Pull: Box not found at expected pre-pull position in current state.");
                }
                break;
            }
            default:
                throw std::invalid_argument("Invalid action type in State constructor");
        }
    }
    // Recalculate hash or clear it so it's lazily recomputed.
    hash_ = 0; 
}

bool State::cellIsFree(const Point2D &position) const {
    if (position.y() < 0 || static_cast<size_t>(position.y()) >= this->level_.walls_.size() || 
        position.x() < 0 || static_cast<size_t>(position.x()) >= this->level_.walls_[0].size()) {
        return false; // Out of bounds
    }
    if (this->level_.walls_[position.y()][position.x()]) return false; // Cell is a wall
    if (agentIdAt(position) != 0) return false;        // Cell is occupied by an agent
    if (boxIdAt(position) != 0) return false;          // Cell is occupied by a box
    return true;
}

char State::agentIdAt(const Point2D &position) const {
    for (const auto &agentPair : this->currentAgents_) { // Iterate current state's agents
        if (agentPair.second.position() == position) {
            return agentPair.first;
        }
    }
    return 0; // No agent at this position
}

char State::boxIdAt(const Point2D &position) const {
    for (const auto &boxPair : this->currentBoxes_) { // Iterate current state's boxes
        if (boxPair.second.position() == position) {
            return boxPair.first;
        }
    }
    return 0; // No box at this position
}

std::vector<char> State::getAgentChars() const {
    std::vector<char> agent_chars;
    agent_chars.reserve(this->currentAgents_.size()); // Use currentAgents_
    for (const auto& pair : this->currentAgents_) {    // Use currentAgents_
        agent_chars.push_back(pair.first);
    }
    std::sort(agent_chars.begin(), agent_chars.end()); 
    return agent_chars;
}

std::vector<char> State::getBoxChars() const {
    std::vector<char> box_chars;
    box_chars.reserve(this->currentBoxes_.size()); // Use currentBoxes_
    for (const auto& pair : this->currentBoxes_) {   // Use currentBoxes_
        box_chars.push_back(pair.first);
    }
    std::sort(box_chars.begin(), box_chars.end()); 
    return box_chars;
}

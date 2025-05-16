#include "state.hpp"

#include <random>

#include "feature_flags.hpp"
#include "helpers.hpp"
#include <vector> // Make sure it's included for std::vector<std::vector<char>>

std::random_device State::rd_;
std::mt19937 State::g_rd_(rd_());

State::State(const Level& level_ref) 
    : level_(level_ref), 
      currentAgents_(level_ref.agentsMap.begin(), level_ref.agentsMap.end()), 
      currentBoxes_(level_ref.boxesMap.begin(), level_ref.boxesMap.end()), 
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

    // --- OPTIMIZATION: Create and populate occupancy grid once for this state expansion ---
    std::vector<std::vector<char>> current_occupancy_grid(this->level_.rows_, std::vector<char>(this->level_.cols_));
    this->populateOccupancyGrid(current_occupancy_grid); // Populate it with current state info
    // --- END OCCUPANCY GRID SETUP ---

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
            // Call the new private isApplicable with the precomputed grid
            if (isApplicable(agent, *actionPtr, current_occupancy_grid)) { 
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
        
        // Call the new private isConflicting with the precomputed grid
        if (!isConflicting(currentJointAction, current_occupancy_grid)) { 
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

    // Hash based on current agent positions using pre-sorted IDs from level_
    for(char agent_id : this->level_.sortedAgentIds_){
        // It's crucial that currentAgents_ map contains all agent_ids from sortedAgentIds_
        // If an agent might not exist in currentAgents_ (e.g., if agents can be removed),
        // you'd need to check for existence with .count() or .find() first.
        // Assuming all agents listed in level_.sortedAgentIds_ are always in currentAgents_:
        utils::hashCombine(seed, this->currentAgents_.at(agent_id).position());
        // Optionally include other agent properties if they define the state and can change
        // utils::hashCombine(seed, this->currentAgents_.at(agent_id).getId()); 
    }

    // Hash based on current box positions using pre-sorted IDs from level_
    for(char box_id : this->level_.sortedBoxIds_){
        // Similar assumption for boxes: all box_ids in level_.sortedBoxIds_ exist in currentBoxes_.
        utils::hashCombine(seed, this->currentBoxes_.at(box_id).position());
        // Optionally include other box properties if they define the state and can change
        // utils::hashCombine(seed, this->currentBoxes_.at(box_id).getId());
        // utils::hashCombine(seed, static_cast<int>(this->currentBoxes_.at(box_id).color()));
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

// New private helper method to populate the occupancy grid
void State::populateOccupancyGrid(std::vector<std::vector<char>>& grid) const {
    const int num_rows = this->level_.rows_;
    const int num_cols = this->level_.cols_;

    // Ensure grid is correctly sized, or clear and resize if it might be reused
    // For now, assume it comes in correctly sized (e.g. getExpandedStates will size it)
    // If not, add: grid.assign(num_rows, std::vector<char>(num_cols, ' '));
    // For safety, let's resize it here.
    grid.assign(num_rows, std::vector<char>(num_cols, ' ')); // ' ' for empty

    // 1. Mark walls
    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            if (this->level_.walls_[r][c]) {
                grid[r][c] = 'W'; // Wall
            }
        }
    }

    // 2. Mark current boxes
    for (const auto& box_pair : this->currentBoxes_) {
        const Point2D& pos = box_pair.second.position();
        if (pos.y() >= 0 && pos.y() < num_rows && pos.x() >= 0 && pos.x() < num_cols) {
            grid[pos.y()][pos.x()] = 'B'; // Box
        }
    }

    // 3. Mark current agents (agents occupy cells over boxes if at same location)
    for (const auto& agent_pair : this->currentAgents_) {
        const Point2D& pos = agent_pair.second.position();
        if (pos.y() >= 0 && pos.y() < num_rows && pos.x() >= 0 && pos.x() < num_cols) {
            grid[pos.y()][pos.x()] = 'A'; // Agent
        }
    }
}

bool State::isApplicable(const Agent &agent_from_current_state, const Action &action) const {
    // This is the original public version. 
    // For now, it could create a grid and call the new private one, or we assume it won't be used externally much.
    // Let's make it create a grid and call the new one to keep it functional if called.
    std::vector<std::vector<char>> occupancy_grid(this->level_.rows_, std::vector<char>(this->level_.cols_));
    populateOccupancyGrid(occupancy_grid);
    return isApplicable(agent_from_current_state, action, occupancy_grid);
}

// New private isApplicable that uses the precomputed grid
bool State::isApplicable(const Agent &agent_from_current_state, const Action &action, const std::vector<std::vector<char>>& occupancy_grid) const {
    Point2D agentPos = agent_from_current_state.position();
    Color agentColor = agent_from_current_state.color(); // Needed for Push/Pull color checks
    const int num_rows = this->level_.rows_;
    const int num_cols = this->level_.cols_;

    Point2D agent_target_pos = agentPos + action.agentDelta;

    // Boundary checks for agent's target position
    if (agent_target_pos.y() < 0 || agent_target_pos.y() >= num_rows || agent_target_pos.x() < 0 || agent_target_pos.x() >= num_cols) {
        return false; // Agent moves out of bounds
    }

    char cell_content_at_agent_target = occupancy_grid[agent_target_pos.y()][agent_target_pos.x()];

    if (action.type == ActionType::Move) {
        // Agent cannot move into a wall or a cell occupied by another agent or a box.
        // The occupancy_grid has agents marked as 'A', boxes as 'B'.
        // The moving agent itself is not yet in agent_target_pos in this grid.
        if (cell_content_at_agent_target == 'W' || 
            cell_content_at_agent_target == 'A' || 
            cell_content_at_agent_target == 'B') {
            return false;
        }
    } else if (action.type == ActionType::Push) {
        if (cell_content_at_agent_target != 'B') return false; // Must push a box

        // Find the box at agent_target_pos
        char box_id_to_push = 0;
        for(const auto& box_pair : this->currentBoxes_){
            if(box_pair.second.position() == agent_target_pos){
                box_id_to_push = box_pair.first;
                if (box_pair.second.color() != agentColor) return false; // Color mismatch
                break;
            }
        }
        if (box_id_to_push == 0) return false; // Should not happen if cell_content_at_agent_target was 'B' from a valid box

        Point2D box_target_pos = agent_target_pos + action.boxDelta; // Box moves in the same dir as agent for Push

        // Boundary checks for box's target position
        if (box_target_pos.y() < 0 || box_target_pos.y() >= num_rows || box_target_pos.x() < 0 || box_target_pos.x() >= num_cols) {
            return false; // Box pushed out of bounds
        }
        // Box cannot be pushed into a Wall, another Agent, or another Box.
        char cell_content_at_box_target = occupancy_grid[box_target_pos.y()][box_target_pos.x()];
        if (cell_content_at_box_target == 'W' || 
            cell_content_at_box_target == 'A' || 
            cell_content_at_box_target == 'B') { 
            return false;
        }
    } else if (action.type == ActionType::Pull) {
        Point2D box_original_pos = agentPos - action.boxDelta; // Box is behind the agent relative to action
        
        // Boundary checks for box's original position
        if (box_original_pos.y() < 0 || box_original_pos.y() >= num_rows || box_original_pos.x() < 0 || box_original_pos.x() >= num_cols) {
            return false; // Box pulled from out of bounds (should not happen if state is valid)
        }
        char cell_content_at_box_original = occupancy_grid[box_original_pos.y()][box_original_pos.x()];
        if (cell_content_at_box_original != 'B') return false; // Must pull a box

        // Find the box at box_original_pos
        char box_id_to_pull = 0;
        for(const auto& box_pair : this->currentBoxes_){
            if(box_pair.second.position() == box_original_pos){
                box_id_to_pull = box_pair.first;
                if (box_pair.second.color() != agentColor) return false; // Color mismatch
                break;
            }
        }
        if (box_id_to_pull == 0) return false; 

        // Agent target cell must be free of walls, other agents, or other boxes (not the one being pulled).
        if (cell_content_at_agent_target == 'W' || 
            cell_content_at_agent_target == 'A' || 
            (cell_content_at_agent_target == 'B' && agent_target_pos != box_original_pos) ) { // If target is a box, it must be the one we are pulling (which it can't be as agent moves into its old cell)
            return false;
        }
    } else if (action.type == ActionType::NoOp) {
        return true; // NoOp is always applicable if all other actions fail
    }

    return true;
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
    // Original public version. Can be updated like isApplicable or removed.
    std::vector<std::vector<char>> occupancy_grid(this->level_.rows_, std::vector<char>(this->level_.cols_));
    populateOccupancyGrid(occupancy_grid);
    return isConflicting(jointAction, occupancy_grid);
}

// New private isConflicting that uses the precomputed grid for static checks
bool State::isConflicting(const std::vector<const Action *> &jointAction, const std::vector<std::vector<char>>& occupancy_grid) const {
    size_t num_agents = jointAction.size();
    if (num_agents == 0) return false;

    // Agent IDs in the order of jointAction (assumed to be sorted from getExpandedStates)
    std::vector<char> agent_ids_in_order; 
    agent_ids_in_order.reserve(this->currentAgents_.size());
    for(const auto& agentPair : this->currentAgents_) { 
        agent_ids_in_order.push_back(agentPair.first);
    }
    std::sort(agent_ids_in_order.begin(), agent_ids_in_order.end());
    if (agent_ids_in_order.size() != num_agents) {
        // This would be an internal error, jointAction size mismatch with currentAgents
        fprintf(stderr, "Error: isConflicting - jointAction size mismatch with current agent count.\n");
        return true; // Treat as conflict
    }


    std::vector<Point2D> agent_prev_positions(num_agents);
    std::vector<Point2D> agent_final_positions(num_agents);
    std::vector<Point2D> box_prev_positions; // Store {id, prev_pos}
    std::vector<Point2D> box_final_positions;  // Store {id, final_pos}
    std::vector<char> moved_box_ids;       // IDs of boxes moved by this joint action

    const int num_rows = this->level_.rows_;
    const int num_cols = this->level_.cols_;

    for (size_t i = 0; i < num_agents; ++i) {
        char agent_id = agent_ids_in_order[i];
        const Agent& agent = this->currentAgents_.at(agent_id);
        const Action* action = jointAction[i];

        agent_prev_positions[i] = agent.position();
        agent_final_positions[i] = agent.position() + action->agentDelta;

        // Check for agent moving out of bounds or into a wall (using the precomputed grid)
        if (agent_final_positions[i].y() < 0 || agent_final_positions[i].y() >= num_rows || 
            agent_final_positions[i].x() < 0 || agent_final_positions[i].x() >= num_cols || 
            occupancy_grid[agent_final_positions[i].y()][agent_final_positions[i].x()] == 'W') {
            return true; // Conflict: Agent moves off board or into wall
        }

        if (action->type == ActionType::Push) {
            Point2D box_orig_pos = agent.position() + action->agentDelta;
            Point2D box_final_pos = box_orig_pos + action->boxDelta;
            
            // Check if box pushed out of bounds or into a wall (using precomputed grid)
            if (box_final_pos.y() < 0 || box_final_pos.y() >= num_rows || 
                box_final_pos.x() < 0 || box_final_pos.x() >= num_cols ||
                occupancy_grid[box_final_pos.y()][box_final_pos.x()] == 'W') {
                return true; // Conflict: Box pushed off board or into wall
            }
            // Also check if box pushed into another agent's initial position (not covered by static grid)
            // This specific check might be better handled by destination cell comparisons later.

            // Find the ID of the box being pushed
            char box_id = 0;
            for(const auto& box_pair : this->currentBoxes_){
                if(box_pair.second.position() == box_orig_pos){
                    box_id = box_pair.first;
                    break;
                }
            }
            if(box_id != 0) { // Should always find a box if action is Push and applicable
                moved_box_ids.push_back(box_id);
                box_prev_positions.push_back(box_orig_pos);
                box_final_positions.push_back(box_final_pos);
            }

        } else if (action->type == ActionType::Pull) {
            Point2D box_orig_pos = agent.position() - action->boxDelta; // Box is behind agent
            Point2D box_final_pos = agent.position(); // Box moves to where agent was

            // (Bounds check for box_orig_pos is part of isApplicable)
            // Check if box pulled into a wall (at agent's original spot - but agent is moving from there)
            // The cell agent_final_positions[i] is where the agent moves.
            // The cell agent_prev_positions[i] is where the box moves.
            // Check if box_final_pos (agent_prev_positions[i]) is a wall is implicitly handled
            // by the agent not being on a wall initially.
            // What matters is if agent_final_positions[i] is a wall (checked above).

            // Find the ID of the box being pulled
            char box_id = 0;
            for(const auto& box_pair : this->currentBoxes_){
                if(box_pair.second.position() == box_orig_pos){
                    box_id = box_pair.first;
                    break;
                }
            }
             if(box_id != 0) { // Should always find a box if action is Pull and applicable
                moved_box_ids.push_back(box_id);
                box_prev_positions.push_back(box_orig_pos);
                box_final_positions.push_back(box_final_pos);
            }
        }
    }

    // Conflict Check 1: Two agents move to the same cell.
    for (size_t i = 0; i < num_agents; ++i) {
        for (size_t j = i + 1; j < num_agents; ++j) {
            if (agent_final_positions[i] == agent_final_positions[j]) {
                return true; // Conflict: Two agents to same cell
            }
        }
    }

    // Conflict Check 2: Agent moves into a cell where a (different) box is moving.
    // Conflict Check 3: Two boxes move to the same cell.
    for (size_t i = 0; i < box_final_positions.size(); ++i) {
        // Check against other boxes
        for (size_t j = i + 1; j < box_final_positions.size(); ++j) {
            if (box_final_positions[i] == box_final_positions[j]) {
                return true; // Conflict: Two boxes to same cell
            }
        }
        // Check against agents
        for (size_t k = 0; k < num_agents; ++k) {
            if (agent_final_positions[k] == box_final_positions[i]) {
                // This is a conflict unless agent k is the one moving box i.
                // Check if agent k is pushing/pulling box moved_box_ids[i]
                bool agent_moved_this_box = false;
                const Action* agent_k_action = jointAction[k];
                if (agent_k_action->type == ActionType::Push) {
                    Point2D box_pushed_by_k_orig_pos = agent_prev_positions[k] + agent_k_action->agentDelta;
                    if (box_pushed_by_k_orig_pos == box_prev_positions[i]) agent_moved_this_box = true;
                } else if (agent_k_action->type == ActionType::Pull) {
                    Point2D box_pulled_by_k_orig_pos = agent_prev_positions[k] - agent_k_action->boxDelta;
                    if (box_pulled_by_k_orig_pos == box_prev_positions[i]) agent_moved_this_box = true;
                }
                if (!agent_moved_this_box) return true; // Conflict
            }
        }
    }

    // Conflict Check 4: Agent moves into a cell occupied by a box that ISN'T moving.
    // The occupancy_grid contains initial positions of static boxes.
    for (size_t i = 0; i < num_agents; ++i) {
        Point2D agent_dest = agent_final_positions[i];
        if (occupancy_grid[agent_dest.y()][agent_dest.x()] == 'B') {
            // Agent moves to a cell that initially had a box. Is this box one that moved?
            bool conflict_with_static_box = true;
            for(const auto& moved_box_prev_pos : box_prev_positions){
                if(moved_box_prev_pos == agent_dest){
                    conflict_with_static_box = false; // The box at agent's dest moved away
                    break;
                }
            }
            if(conflict_with_static_box) return true; // Conflict with a box that didn't move
        }
    }
    
    // Conflict Check 5: Box moves into a cell occupied by another box that ISN'T moving.
    // This is harder with the current structure. If a box B1 moves to P, and P initially had static box B2,
    // this is a conflict. occupancy_grid[box_final_pos.y()][box_final_pos.x()] == 'B' implies this.
    // However, we also need to ensure that the 'B' is not the previous position of *another moved box*.
    for(const auto& box_dest : box_final_positions) {
        if (occupancy_grid[box_dest.y()][box_dest.x()] == 'B') {
            bool conflict_with_static_box = true;
            for(const auto& moved_box_prev_pos : box_prev_positions) {
                 if (moved_box_prev_pos == box_dest) { // The box at box_dest moved away.
                     conflict_with_static_box = false;
                     break;
                 }
            }
            if (conflict_with_static_box) return true;
        }
    }

    // TODO: Add check for agent moving to the previous location of another agent (swap places)
    // For actions that are not NoOp, if agent_final_pos[i] == agent_prev_pos[j] AND agent_final_pos[j] == agent_prev_pos[i]
    // This is usually allowed. The current checks for final positions colliding are primary.

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

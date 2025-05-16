std::pair<int, std::vector<const Action*>> SumOfIndividualCostsCalculator::calculateSingleAgentPath(
    const State& state,
    char agent_id,
    const std::function<int(ActionType)>& action_cost_fn,
    const Level& level_ref
) const {
    auto agent_it = state.currentAgents_.find(agent_id); 
    if (agent_it == state.currentAgents_.end()) {
        // ... existing code ...
    }
} 
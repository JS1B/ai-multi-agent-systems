#include "state.h"

#include <iostream>
#include <algorithm>

// Static member initialization
std::vector<int> State::agentRows;
std::vector<int> State::agentCols;
std::vector<Color> State::agentColors;
std::vector<std::vector<bool>> State::walls;
std::vector<std::vector<char>> State::boxes;
std::vector<Color> State::boxColors;
std::vector<std::vector<char>> State::goals;

State::State(std::vector<int> agentRows, std::vector<int> agentCols, std::vector<Color> agentColors,
             std::vector<std::vector<bool>> walls, std::vector<std::vector<char>> boxes, std::vector<Color> boxColors,
             std::vector<std::vector<char>> goals)
{
    this->agentRows = agentRows;
    this->agentCols = agentCols;
    this->agentColors = agentColors;
    this->walls = walls;
    this->boxes = boxes;
    this->boxColors = boxColors;
    this->goals = goals;
    parent = nullptr;
    g = 0;
}

std::vector<std::vector<Action>> State::extractPlan()
{
    std::vector<std::vector<Action>> plan;
    State *current = this;
    while (current->parent != nullptr)
    {
        plan.insert(plan.begin(), current->jointAction); // Add to the beginning to reverse the order
        current = current->parent;
    }
    return plan;
}

std::vector<State> State::getExpandedStates()
{
    size_t numAgents = agentRows.size();
    std::vector<State> expandedStates;

    // Determine list of applicable actions for each individual agent
    std::vector<std::vector<Action>> applicableActions(numAgents);
    for (size_t agent = 0; agent < numAgents; ++agent)
    {
        for (const Action &action : Action::ALL_ACTIONS)
        {
            if (isApplicable(agent, action))
            {
                applicableActions[agent].push_back(action);
            }
        }
    }

    // Iterate over joint actions, check conflict and generate child states
    std::vector<Action> jointAction(numAgents, Action::NoOp);
    std::vector<size_t> actionsPermutation(numAgents, 0);

    while (true)
    {
        // Create joint action from current permutation
        for (size_t agent = 0; agent < numAgents; ++agent)
        {
            jointAction[agent] = applicableActions[agent][actionsPermutation[agent]];
        }

        // If joint action is not conflicting, create new state
        if (!isConflicting(jointAction))
        {
            State newState = result(jointAction);
            expandedStates.push_back(newState);
        }

        // Advance permutation
        bool done = false;
        for (size_t agent = 0; agent < numAgents; ++agent)
        {
            if (actionsPermutation[agent] < applicableActions[agent].size() - 1)
            {
                actionsPermutation[agent]++;
                break;
            }
            else
            {
                actionsPermutation[agent] = 0;
                if (agent == numAgents - 1)
                {
                    done = true;
                }
            }
        }

        if (done)
            break;
    }

    // Shuffle expanded states
    std::random_shuffle(expandedStates.begin(), expandedStates.end());
    return expandedStates;
}

bool State::isApplicable(int agent, const Action &action)
{
    int agentRow = agentRows[agent];
    int agentCol = agentCols[agent];

    if (action.type == ActionType::NoOp)
    {
        return true;
    }

    if (action.type == ActionType::Move)
    {
        int destinationRow = agentRow + action.agentRowDelta;
        int destinationCol = agentCol + action.agentColDelta;
        return cellIsFree(destinationRow, destinationCol);
    }

    return false;
}

bool State::isConflicting(const std::vector<Action> &jointAction)
{
    size_t numAgents = agentRows.size();
    std::vector<int> destinationRows(numAgents, -1);
    std::vector<int> destinationCols(numAgents, -1);
    std::vector<int> boxRows(numAgents, -1);
    std::vector<int> boxCols(numAgents, -1);

    // Collect cells to be occupied and boxes to be moved
    for (size_t agent = 0; agent < numAgents; ++agent)
    {
        const Action &action = jointAction[agent];
        int agentRow = agentRows[agent];
        int agentCol = agentCols[agent];

        if (action.type == ActionType::NoOp)
        {
            continue;
        }
        else if (action.type == ActionType::Move)
        {
            destinationRows[agent] = agentRow + action.agentRowDelta;
            destinationCols[agent] = agentCol + action.agentColDelta;
            boxRows[agent] = agentRow; // Distinct dummy value
            boxCols[agent] = agentCol; // Distinct dummy value
        }
    }

    // Check conflicts
    for (size_t a1 = 0; a1 < numAgents; ++a1)
    {
        if (jointAction[a1].type == ActionType::NoOp)
        {
            continue;
        }

        for (size_t a2 = a1 + 1; a2 < numAgents; ++a2)
        {
            if (jointAction[a2].type == ActionType::NoOp)
            {
                continue;
            }

            // Moving into same cell?
            if (destinationRows[a1] == destinationRows[a2] &&
                destinationCols[a1] == destinationCols[a2])
            {
                return true;
            }
        }
    }

    return false;
}

State State::result(const std::vector<Action> &jointAction)
{
    // Create a copy of the current state
    State newState = *this;

    // Apply each action
    for (size_t agent = 0; agent < jointAction.size(); ++agent)
    {
        const Action &action = jointAction[agent];

        if (action.type == ActionType::NoOp)
        {
            continue;
        }
        else if (action.type == ActionType::Move)
        {
            newState.agentRows[agent] += action.agentRowDelta;
            newState.agentCols[agent] += action.agentColDelta;
        }
    }

    // Update state metadata
    newState.parent = this;
    newState.jointAction = jointAction;
    newState.g = g + 1;

    return newState;
}

bool State::isGoalState()
{
    for (size_t row = 0; row < goals.size(); ++row)
    {
        for (size_t col = 0; col < goals[row].size(); ++col)
        {
            char goal = goals[row][col];

            if ('A' <= goal && goal <= 'Z')
            {
                if (boxes[row][col] != goal)
                {
                    return false;
                }
            }
            else if ('0' <= goal && goal <= '9')
            {
                int agent = goal - '0';
                if (!(agentRows[agent] == row && agentCols[agent] == col))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

int State::getG() const
{
    return g;
}

bool State::operator==(const State &other) const
{
    if (agentRows.size() != other.agentRows.size() || agentCols.size() != other.agentCols.size() || boxes.size() != other.boxes.size())
    {
        return false;
    }

    if (agentRows != other.agentRows || agentCols != other.agentCols)
    {
        return false;
    }

    for (size_t i = 0; i < boxes.size(); ++i)
    {
        if (boxes[i] != other.boxes[i])
        {
            return false;
        }
    }

    return true;
}

bool State::operator<(const State &other) const
{
    if (agentRows.size() != other.agentRows.size())
    {
        return agentRows.size() < other.agentRows.size();
    }

    for (size_t i = 0; i < agentRows.size(); ++i)
    {
        if (agentRows[i] != other.agentRows[i])
        {
            return agentRows[i] < other.agentRows[i];
        }
        if (agentCols[i] != other.agentCols[i])
        {
            return agentCols[i] < other.agentCols[i];
        }
    }

    for (size_t i = 0; i < boxes.size(); ++i)
    {
        if (boxes[i] != other.boxes[i])
        {
            return boxes[i] < other.boxes[i];
        }
    }

    return false;
}

bool State::cellIsFree(int row, int col)
{
    return !walls[row][col] && boxes[row][col] == ' ' && agentAt(row, col) == 0;
}

char State::agentAt(int row, int col)
{
    for (size_t agent = 0; agent < agentRows.size(); ++agent)
    {
        if (agentRows[agent] == row && agentCols[agent] == col)
        {
            return '0' + agent;
        }
    }
    return 0;
}
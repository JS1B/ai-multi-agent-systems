#include "frontier.h"

#include <algorithm>

// FrontierBFS
FrontierBFS::FrontierBFS() {}

void FrontierBFS::add(State state)
{
    queue.push(state);
    set.insert(state);
}

State FrontierBFS::pop()
{
    State state = queue.front();
    queue.pop();
    set.erase(state);
    return state;
}

bool FrontierBFS::isEmpty()
{
    return queue.empty();
}

int FrontierBFS::size()
{
    return queue.size();
}

bool FrontierBFS::contains(const State &state)
{
    return set.count(state) > 0;
}

std::string FrontierBFS::getName()
{
    return "breadth-first search";
}

// FrontierDFS
FrontierDFS::FrontierDFS() {}

void FrontierDFS::add(State state)
{
    stack.push(state);
    set.insert(state);
}

State FrontierDFS::pop()
{
    State state = stack.top();
    stack.pop();
    set.erase(state);
    return state;
}

bool FrontierDFS::isEmpty()
{
    return stack.empty();
}

int FrontierDFS::size()
{
    return stack.size();
}

bool FrontierDFS::contains(const State &state)
{
    return set.count(state) > 0;
}

std::string FrontierDFS::getName()
{
    return "depth-first search";
}

// FrontierBestFirst
FrontierBestFirst::FrontierBestFirst(Heuristic *heuristic) : heuristic(heuristic),
                                                             queue([heuristic](const State &a, const State &b)
                                                                   { return heuristic->f(a) > heuristic->f(b); }) {}

void FrontierBestFirst::add(State state)
{
    queue.push(state);
    set.insert(state);
}

State FrontierBestFirst::pop()
{
    State state = queue.top();
    queue.pop();
    set.erase(state);
    return state;
}

bool FrontierBestFirst::isEmpty()
{
    return queue.empty();
}

int FrontierBestFirst::size()
{
    return queue.size();
}

bool FrontierBestFirst::contains(const State &state)
{
    return set.count(state) > 0;
}

std::string FrontierBestFirst::getName()
{
    return "best-first search using " + heuristic->toString();
}
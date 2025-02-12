#ifndef SEARCHCLIENT_FRONTIER_H
#define SEARCHCLIENT_FRONTIER_H

#include "state.h"
#include "heuristic.h"

#include <queue>
#include <set>
#include <stack>
#include <string>
#include <vector>
#include <functional>

class Frontier
{
public:
    virtual void add(State state) = 0;
    virtual State pop() = 0;
    virtual bool isEmpty() = 0;
    virtual int size() = 0;
    virtual bool contains(const State &state) = 0;
    virtual std::string getName() = 0;
    virtual ~Frontier() = default;
};

class FrontierBFS : public Frontier
{
public:
    FrontierBFS();
    void add(State state) override;
    State pop() override;
    bool isEmpty() override;
    int size() override;
    bool contains(const State &state) override;
    std::string getName() override;

private:
    std::queue<State> queue;
    std::set<State> set;
};

class FrontierDFS : public Frontier
{
public:
    FrontierDFS();
    void add(State state) override;
    State pop() override;
    bool isEmpty() override;
    int size() override;
    bool contains(const State &state) override;
    std::string getName() override;

private:
    std::stack<State> stack;
    std::set<State> set;
};

class FrontierBestFirst : public Frontier
{
public:
    FrontierBestFirst(Heuristic *heuristic);
    void add(State state) override;
    State pop() override;
    bool isEmpty() override;
    int size() override;
    bool contains(const State &state) override;
    std::string getName() override;

private:
    Heuristic *heuristic;
    std::priority_queue<State, std::vector<State>, std::function<bool(const State &, const State &)>> queue;
    std::set<State> set;
};

#endif // SEARCHCLIENT_FRONTIER_H
#ifndef SEARCHCLIENT_HEURISTIC_H
#define SEARCHCLIENT_HEURISTIC_H

#include "state.h"
#include <string>

class Heuristic
{
public:
    Heuristic(State initial_state);
    virtual int h(State state);
    virtual int f(State state) = 0;
    virtual std::string toString() = 0;
    virtual ~Heuristic() = default;
};

class HeuristicAStar : public Heuristic
{
public:
    HeuristicAStar(State initial_state);
    int f(State state) override;
    std::string toString() override;
};

class HeuristicWeightedAStar : public Heuristic
{
public:
    HeuristicWeightedAStar(State initial_state, int w);
    int f(State state) override;
    std::string toString() override;

private:
    int w;
};

class HeuristicGreedy : public Heuristic
{
public:
    HeuristicGreedy(State initial_state);
    int f(State state) override;
    std::string toString() override;
};

#endif // SEARCHCLIENT_HEURISTIC_H
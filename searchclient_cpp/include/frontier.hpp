#pragma once

#include <functional>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include "heuristic.hpp"
#include "state.hpp"

class Frontier {
   public:
    virtual void add(State state) = 0;
    virtual State pop() = 0;
    virtual bool isEmpty() = 0;
    virtual size_t size() = 0;
    virtual bool contains(const State &state) = 0;
    virtual std::string getName() = 0;
    virtual ~Frontier() = default;
};

class FrontierBFS : public Frontier {
   public:
    FrontierBFS() = default;
    void add(State state) override {
        queue_.push(state);
        set_.insert(state);
    }

    State pop() override {
        State state = queue_.front();
        queue_.pop();
        set_.erase(state);
        return state;
    }

    bool isEmpty() override {
        return queue_.empty();
    }

    size_t size() override {
        return queue_.size();
    }

    bool contains(const State &state) override {
        return set_.find(state) != set_.end();
    }

    std::string getName() override {
        return "Breadth-first search";
    }

   private:
    std::queue<State, std::deque<State>> queue_;  // @todo replace with something more efficient
    std::set<State> set_;                         // @todo replace with something more efficient
};

// class FrontierDFS : public Frontier {
//    public:
//     FrontierDFS();
//     void add(State state) override;
//     State pop() override;
//     bool isEmpty() override;
//     int size() override;
//     bool contains(const State &state) override;
//     std::string getName() override;

//    private:
//     std::stack<State> stack;
//     std::set<State> set;
// };

// class FrontierBestFirst : public Frontier {
//    public:
//     FrontierBestFirst(Heuristic *heuristic);
//     void add(State state) override;
//     State pop() override;
//     bool isEmpty() override;
//     int size() override;
//     bool contains(const State &state) override;
//     std::string getName() override;

//    private:
//     Heuristic *heuristic;
//     std::priority_queue<State, std::vector<State>, std::function<bool(const State &, const State &)>> queue;
//     std::set<State> set;
// };

#pragma once

#include <deque>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "heuristic.hpp"
#include "state.hpp"

class Frontier {
   public:
    virtual ~Frontier() = 0;
    virtual void add(State* state) = 0;
    virtual State* pop() = 0;
    virtual bool isEmpty() const = 0;
    virtual size_t size() const = 0;
    virtual bool contains(State* state) const = 0;
    virtual std::string getName() const = 0;
};

// Breadth-First Search Frontier
class FrontierBFS : public Frontier {
   private:
    std::deque<State*> queue_;
    std::unordered_set<State*, StatePtrHash, StatePtrEqual> set_;

   public:
    FrontierBFS() { set_.reserve(100'000); }
    ~FrontierBFS() {
        for (auto state : set_) {
            delete state;
        }
    }

    void add(State* state) override {
        queue_.push_back(state);
        set_.insert(state);
    }

    State* pop() override {
        if (isEmpty()) {
            throw std::runtime_error("Cannot pop from an empty BFS frontier.");
        }
        State* state = queue_.front();
        queue_.pop_front();
        set_.erase(state);
        return state;
    }

    bool isEmpty() const override { return queue_.empty(); }

    size_t size() const override { return queue_.size(); }

    bool contains(State* state) const override { return set_.count(state); }

    std::string getName() const override { return "breadth-first search"; }
};

// Depth-First Search Frontier
class FrontierDFS : public Frontier {
   private:
    std::deque<State*> queue_;
    std::unordered_set<State*, StatePtrHash, StatePtrEqual> set_;

   public:
    FrontierDFS() { set_.reserve(100'000); }
    ~FrontierDFS() {
        for (auto state : set_) {
            delete state;
        }
    }

    void add(State* state) override {
        queue_.push_front(state);
        set_.insert(state);
    }

    State* pop() override {
        if (isEmpty()) {
            throw std::runtime_error("Cannot pop from an empty DFS frontier.");
        }
        State* state = queue_.front();
        queue_.pop_front();
        set_.erase(state);
        return state;
    }

    bool isEmpty() const override { return queue_.empty(); }

    size_t size() const override { return queue_.size(); }

    bool contains(State* state) const override { return set_.count(state); }

    std::string getName() const override { return "depth-first search"; }
};

class FrontierBestFirst : public Frontier {
   private:
    const Heuristic* heuristic_;
    std::unordered_set<State*, StatePtrHash, StatePtrEqual> set_;

    struct StateComparator {
        const Heuristic* h;
        StateComparator(const Heuristic* heuristic) : h(heuristic) {}
        bool operator()(const State* lhs, const State* rhs) const {
            // Higher f-value means lower priority (further down the max-heap)
            return h->f(*lhs) > h->f(*rhs);
        }
    };

    std::priority_queue<State*, std::vector<State*>, StateComparator> queue_;

   public:
    FrontierBestFirst(const Heuristic* h) : heuristic_(h), queue_(StateComparator(h)) {
        if (!heuristic_) {
            throw std::invalid_argument("Heuristic cannot be null for FrontierBestFirst.");
        }
    }

    ~FrontierBestFirst() {
        for (auto state : set_) {
            delete state;
        }
    }

    void add(State* state) override {
        queue_.push(state);
        set_.insert(state);
    }

    State* pop() override {
        if (isEmpty()) {
            throw std::runtime_error("Cannot pop from an empty BestFirst frontier.");
        }
        State* state = queue_.top();
        queue_.pop();
        set_.erase(state);
        return state;
    }

    bool isEmpty() const override { return set_.empty(); }

    size_t size() const override { return set_.size(); }

    bool contains(State* state) const override { return set_.count(state); }

    std::string getName() const override { return "best-first search using " + heuristic_->getName(); }
};

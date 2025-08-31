#pragma once

#include <deque>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "heuristic.hpp"
#include "low_level_state.hpp"

class Frontier {
   public:
    virtual ~Frontier() = default;
    virtual void add(LowLevelState* state) = 0;
    virtual LowLevelState* pop() = 0;
    virtual bool isEmpty() const = 0;
    virtual size_t size() const = 0;
    virtual bool contains(LowLevelState* state) const = 0;
    virtual void clear() = 0;
    virtual std::string getName() const = 0;
};

// Breadth-First Search Frontier
class FrontierBFS : public Frontier {
   private:
    std::deque<LowLevelState*> queue_;
    std::unordered_set<LowLevelState*, LowLevelStatePtrHash, LowLevelStatePtrEqual> set_;

   public:
    FrontierBFS() { set_.reserve(10'000); }
    ~FrontierBFS() {
        for (auto state : set_) {
            delete state;
        }
    }

    void add(LowLevelState* state) override {
        queue_.push_back(state);
        set_.insert(state);
    }

    LowLevelState* pop() override {
        if (isEmpty()) {
            throw std::runtime_error("Cannot pop from an empty BFS frontier.");
        }
        LowLevelState* state = queue_.front();
        queue_.pop_front();
        set_.erase(state);
        return state;
    }

    bool isEmpty() const override { return queue_.empty(); }

    size_t size() const override { return queue_.size(); }

    bool contains(LowLevelState* state) const override { return set_.count(state); }

    void clear() override {
        for (auto state : set_) {
            delete state;
        }
        queue_.clear();
        set_.clear();
    }

    std::string getName() const override { return "breadth-first search"; }
};

// Depth-First Search Frontier
class FrontierDFS : public Frontier {
   private:
    std::deque<LowLevelState*> queue_;
    std::unordered_set<LowLevelState*, LowLevelStatePtrHash, LowLevelStatePtrEqual> set_;

   public:
    FrontierDFS() { set_.reserve(10'000); }
    ~FrontierDFS() {
        for (auto state : set_) {
            delete state;
        }
    }

    void add(LowLevelState* state) override {
        queue_.push_front(state);
        set_.insert(state);
    }

    LowLevelState* pop() override {
        if (isEmpty()) {
            throw std::runtime_error("Cannot pop from an empty DFS frontier.");
        }
        LowLevelState* state = queue_.front();
        queue_.pop_front();
        set_.erase(state);
        return state;
    }

    bool isEmpty() const override { return queue_.empty(); }

    size_t size() const override { return queue_.size(); }

    bool contains(LowLevelState* state) const override { return set_.count(state); }

    void clear() override {
        for (auto state : set_) {
            delete state;
        }
        queue_.clear();
        set_.clear();
    }

    std::string getName() const override { return "depth-first search"; }
};

class FrontierBestFirst : public Frontier {
   private:
    const Heuristic* heuristic_;
    std::unordered_set<LowLevelState*, LowLevelStatePtrHash, LowLevelStatePtrEqual> set_;

    struct StateComparator {
        const Heuristic* heur;
        StateComparator(const Heuristic* heuristic) : heur(heuristic) {}
        bool operator()(const LowLevelState* lhs, const LowLevelState* rhs) const {
            // Higher f-value means lower priority (further down the max-heap)
            return heur->f(*lhs) > heur->f(*rhs);
        }
    };

    std::priority_queue<LowLevelState*, std::vector<LowLevelState*>, StateComparator> queue_;

   public:
    FrontierBestFirst(const Heuristic* heuristic) : heuristic_(heuristic), queue_(StateComparator(heuristic)) {
        if (!heuristic_) {
            throw std::invalid_argument("Heuristic cannot be null for FrontierBestFirst.");
        }
        set_.reserve(1'000);
    }

    ~FrontierBestFirst() {
        for (auto state : set_) {
            delete state;
        }
        delete heuristic_;
    }

    void add(LowLevelState* state) override {
        queue_.push(state);
        set_.insert(state);
    }

    LowLevelState* pop() override {
        if (isEmpty()) {
            throw std::runtime_error("Cannot pop from an empty BestFirst frontier.");
        }
        LowLevelState* state = queue_.top();
        queue_.pop();
        set_.erase(state);
        return state;
    }

    bool isEmpty() const override { return set_.empty(); }

    size_t size() const override { return set_.size(); }

    bool contains(LowLevelState* state) const override { return set_.count(state); }

    void clear() override {
        for (auto state : set_) {
            delete state;
        }
        // Clear the priority_queue by creating a new empty one
        queue_ = std::priority_queue<LowLevelState*, std::vector<LowLevelState*>, StateComparator>(StateComparator(heuristic_));
        set_.clear();
    }

    std::string getName() const override { return "best-first search using " + heuristic_->getName(); }
};

#pragma once

#include <deque>
#include <functional>  // For std::hash, std::function
#include <memory>      // For std::unique_ptr
#include <queue>
#include <stdexcept>  // For std::runtime_error
#include <string>
#include <unordered_set>
#include <vector>

#include "heuristic.hpp"
#include "state.hpp"

// Abstract Frontier base class (Interface)
class Frontier {
   public:
    virtual ~Frontier() = default;
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
    std::unordered_set<State*, State::hash> set_;

   public:
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
    std::unordered_set<State*, State::hash> set_;

   public:
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

// @todo gpt below - completely untested
// Best-First Search Frontier (A*, Greedy Best-First, Uniform Cost)
// class FrontierBestFirst : public Frontier {
//    private:
//     const Heuristic* heuristic_;  // Pointer to the heuristic, ownership managed externally
//     std::unordered_set<State, State::hash> set_;

//     // Custom comparator for the priority queue
//     // Compares states based on the heuristic's f() value
//     // std::priority_queue is a max-heap by default, so we need > for a min-heap behavior based on f()
//     struct StateComparator {
//         const Heuristic* h;
//         StateComparator(const Heuristic* heuristic) : h(heuristic) {}
//         bool operator()(const State& lhs, const State& rhs) const {
//             // Higher f-value means lower priority (further down the max-heap)
//             return h->f(lhs) > h->f(rhs);
//         }
//     };

//     // Priority queue using the custom comparator
//     std::priority_queue<State, std::vector<State>, StateComparator> queue;

//    public:
//     // Constructor takes a pointer to a Heuristic object
//     FrontierBestFirst(const Heuristic* h) : heuristic_(h), queue_(StateComparator(h)) {
//         if (!heuristic_) {
//             throw std::invalid_argument("Heuristic cannot be null for FrontierBestFirst.");
//         }
//     }

//     void add(const State* state) override {
//         queue_.push(*state);
//         set_.insert(*state);
//     }

//     State* pop() override {
//         if (isEmpty()) {
//             throw std::runtime_error("Cannot pop from an empty BestFirst frontier.");
//         }
//         // top() returns a const reference, need to copy before popping
//         State state = queue_.top();
//         queue_.pop();
//         set_.erase(state);  // Erase requires a non-const State, relies on State's hash and ==
//         return state;
//     }

//     // Check the set for emptiness, as queue.empty() might be true while set isn't (if pop failed somehow?)
//     // Or just rely on queue emptiness which should be consistent. Java used set.isEmpty().
//     bool isEmpty() const override { return queue_.empty(); }

//     // Size based on the queue size
//     size_t size() const override { return queue_.size(); }

//     bool contains(const State* state) const override { return set_.count(*state); }

//     std::string getName() const override { ic interface return "best-first search using " + heuristic_->getName(); }
// };

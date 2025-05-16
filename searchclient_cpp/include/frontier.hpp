#pragma once

#include <deque>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include <map>

#include "third_party/emhash/hash_table8.hpp"

#include "heuristic.hpp"
#include "state.hpp"

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
    std::unordered_set<State*, StatePtrHash, StatePtrEqual> set_;

   public:
    FrontierBFS() { set_.reserve(10'000); }

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
    FrontierDFS() { set_.reserve(10'000); }

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
class FrontierBestFirst : public Frontier {
   public:
    FrontierBestFirst(Heuristic *heuristic) : heuristic_(heuristic) {
        if (!heuristic_) {
            throw std::invalid_argument("Heuristic cannot be null for FrontierBestFirst.");
        }
    }
    ~FrontierBestFirst() override {
        delete heuristic_; // Owns the heuristic object
        // States in open_set_ and closed_set_ are owned by the GraphSearch (caller of pop)
        // or StateMemoryPoolAllocator if custom new/delete is fully utilized.
        // For now, assuming states popped are deleted by caller if no plan, or part of plan.
        // States not popped from open_set_ upon destruction should be cleaned up if not managed by pool.
        for (auto const& [key, val] : open_set_) {
            delete val; // Clean up states remaining in open_set
        }
    }

    void add(State *state) override {
        int f_value = heuristic_->f(*state);

        // Check if already in closed set
        auto closed_it = closed_set_.find(state);
        if (closed_it != closed_set_.end()) {
            // If in closed and the path to it was better or equal, ignore new state.
            // (This simple check uses f-value; could use g-value if heuristic is consistent)
            if (closed_it->second <= f_value) { 
                delete state; // New path is not better
                return;
            }
            // If new path is better (f_value is lower), it shouldn't be in closed yet with a worse value.
            // This indicates a potential issue or need for reopening. For now, we don't reopen.
        }

        // Check if a version of this state is in open_set_ already
        // This is inefficient for multimap. A common A* uses a map for open_set_ from State* to f-value/iterator
        // to allow efficient update. For simplicity with multimap, we might add duplicates
        // and let pop() handle finding the true best one. Or, iterate to find and update.
        // Iterating to find and update (if better path found):
        bool updated_in_open = false;
        for (auto it = open_set_.begin(); it != open_set_.end(); ++it) {
            if (*(it->second) == *state) { // State content equality
                if (f_value < it->first) { // New path is better
                    delete it->second; // Delete old state from open set
                    open_set_.erase(it);
                    open_set_.emplace(f_value, state); // Add new one
                    updated_in_open = true;
                    break;
                } else {
                    delete state; // New path not better than one already in open
                    updated_in_open = true; // Mark as handled
                    break;
                }
            }
        }

        if (!updated_in_open) {
            open_set_.emplace(f_value, state);
        }
    }

    State *pop() override {
        if (isEmpty()) {
            throw std::out_of_range("Cannot pop from an empty BestFirst frontier.");
        }
        auto best_it = open_set_.begin(); // Smallest f-value is at the beginning of multimap
        State *best_state = best_it->second;
        int f_value = best_it->first;
        open_set_.erase(best_it);

        closed_set_.emplace(best_state, f_value); // Add to closed set
        return best_state;
    }

    bool isEmpty() const override { return open_set_.empty(); }
    size_t size() const override { return open_set_.size(); }
    // Contains for BestFirst typically means if it's scheduled for expansion (in open) 
    // or already expanded (in closed). This is a simplified 'is in open_set'.
    bool contains(State *state) const override { 
        for (auto const& [key, val] : open_set_) {
            if (*val == *state) return true;
        }
        return false; 
    }
    std::string getName() const override { return heuristic_->getName(); }
    const Heuristic* getHeuristic() const { return heuristic_; }

private:
    Heuristic *heuristic_;
    std::multimap<int, State *> open_set_; // Key: f-value, Value: State pointer
    emhash8::HashMap<const State *, int, StatePtrHash, StatePtrEqual> closed_set_; // Key: State pointer, Value: f-value when closed
};

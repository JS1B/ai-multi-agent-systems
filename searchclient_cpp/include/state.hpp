#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <new>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "action.hpp"
#include "color.hpp"
#include "level.hpp"

class State {
   public:
    State() = delete;
    State(Level level);

    Level level;

    [[nodiscard]] static void *operator new(std::size_t number_of_states);
    static void operator delete(void *ptr) noexcept;

    const State *parent;
    std::vector<const Action *> jointAction;

    inline int getG() const { return g_; }

    std::vector<std::vector<const Action *>> extractPlan() const;
    std::vector<State *> getExpandedStates() const;

    size_t getHash() const;
    bool operator==(const State &other) const;
    bool operator<(const State &other) const;

    bool isGoalState() const;
    bool isApplicable(const Agent &agent, const Action &action) const;
    bool isConflicting(const std::vector<const Action *> &jointAction) const;

    std::string toString() const;

   private:
    // Creates a new state from a parent state and a joint action performed in that state.
    State(const State *parent, std::vector<const Action *> jointAction);

    const int g_;  // Cost of reaching this state
    mutable size_t hash_ = 0;
    // Store random device for shuffling
    static std::random_device rd_;
    static std::mt19937 g_rd_;

    // Returns true if the cell at the given position is free (i.e. not a wall, box, or agent)
    bool cellIsFree(const Point2D &position) const;

    // Returns the id of the agent at the given position, or 0
    char agentIdAt(const Point2D &position) const;

    // Returns the id of the box at the given position, or 0
    char boxIdAt(const Point2D &position) const;
};

// Custom hash function for State pointers
struct StatePtrHash {
    std::size_t operator()(const State *state_ptr) const {
        if (!state_ptr) {
            return 0;
        }
        return state_ptr->getHash();
    }
};

// Custom equality operator for State pointers
struct StatePtrEqual {
    bool operator()(const State *lhs_ptr, const State *rhs_ptr) const {
        // Handle null pointers
        if (lhs_ptr == rhs_ptr) {  // Both null or same pointer
            return true;
        }
        if (!lhs_ptr || !rhs_ptr) {  // One is null, the other isn't
            return false;
        }
        // Dereference pointers and use the State's equality operator
        return *lhs_ptr == *rhs_ptr;
    }
};

// Insert StateMemoryPoolAllocator class definition here
class StateMemoryPoolAllocator {
   private:
    static constexpr size_t ESTIMATED_STATES_IN_POOL = 10'000'000;
    static constexpr size_t STATE_SIZE_FOR_POOL = sizeof(State);
    static constexpr size_t POOL_SIZE_BYTES = ESTIMATED_STATES_IN_POOL * STATE_SIZE_FOR_POOL;

    alignas(alignof(State)) std::vector<u_int8_t> memory_pool_;
    size_t next_free_offset_ = 0;

    StateMemoryPoolAllocator() {
        try {
            memory_pool_.resize(POOL_SIZE_BYTES);
        } catch (const std::bad_alloc &e) {
            fprintf(stderr, "CRITICAL ERROR: Failed to allocate memory pool of %zu bytes for StateMemoryPoolAllocator.\n", POOL_SIZE_BYTES);
            fprintf(stderr, "sizeof(State) is %zu bytes.\n", sizeof(State));
            throw;  // Re-throw, program likely cannot continue
        }
        // fprintf(stderr, "INFO: StateMemoryPoolAllocator pool initialized. Capacity: %zu bytes for approx %zu states.\n",
        //         memory_pool_.capacity(), memory_pool_.capacity() / (sizeof(State) > 0 ? sizeof(State) : 1));
    }

    StateMemoryPoolAllocator(const StateMemoryPoolAllocator &) = delete;
    StateMemoryPoolAllocator &operator=(const StateMemoryPoolAllocator &) = delete;

   public:
    static StateMemoryPoolAllocator &getInstance() {
        static StateMemoryPoolAllocator instance;
        return instance;
    }

    // Allocate memory for one State object from the pool
    void *allocate(std::size_t size) {
        // Ensure size is padded to be a multiple of alignment for subsequent allocations
        // This is a simplified approach; robust padding might be slightly more complex
        // if size itself isn't already suitable.
        // std::size_t aligned_size = (size + alignof(State) - 1) & ~(alignof(State) - 1);
        // if you want to pad size itself
        // but usually sizeof(T) is used directly

        std::size_t current_offset_aligned = (next_free_offset_ + alignof(State) - 1) & ~(alignof(State) - 1);

        if (current_offset_aligned + size > memory_pool_.size()) {  // Use original size for check after aligning start
            fprintf(stderr, "CRITICAL ERROR: StateMemoryPoolAllocator pool exhausted...\n");
            throw std::bad_alloc();
        }
        void *ptr = memory_pool_.data() + current_offset_aligned;
        next_free_offset_ = current_offset_aligned + size;  // Use original size for increment
        return ptr;
    }

    // @todo: Currently no-op, but could be used to return memory to the pool if needed.
    void deallocate(void *ptr) noexcept {
        (void)ptr;  // Suppress unused parameter warning
    }

    // Optional: Reset for multiple runs (if your main loop runs search multiple times)
    void reset() {
        fprintf(stderr, "INFO: StateAllocator pool reset.\n");
        next_free_offset_ = 0;
        // For debugging, you might want to fill with a pattern:
        // std::fill(memory_pool_.begin(), memory_pool_.end(), 0xCD);
    }

    inline size_t getUsedBytes() const { return next_free_offset_; }
    inline size_t getTotalBytes() const { return memory_pool_.capacity(); }
    inline size_t getRemainingBytes() const { return memory_pool_.capacity() - next_free_offset_; }
};

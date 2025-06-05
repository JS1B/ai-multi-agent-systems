// #pragma once

// #include <algorithm>
// #include <cassert>
// #include <cstddef>
// #include <functional>
// #include <new>
// #include <random>
// #include <sstream>
// #include <string>
// #include <vector>

// #include "action.hpp"
// #include "color.hpp"
// #include "level.hpp"

// class CBSState {
//    public:
//     CBSState() = delete;
//     CBSState(Level level);

//     Level level;

//     [[nodiscard]] static void *operator new(std::size_t number_of_states);
//     static void operator delete(void *ptr) noexcept;

//     const CBSState *parent;
//     std::vector<const Action *> jointAction;

//     inline int getG() const { return g_; }

//     std::vector<std::vector<const Action *>> extractPlan() const;
//     std::vector<CBSState *> getExpandedStates() const;

//     size_t getHash() const;
//     bool operator==(const CBSState &other) const;
//     bool operator<(const CBSState &other) const;

//     bool isGoalState() const;
//     bool isApplicable(int agent_idx, const Action &action) const;
//     bool isConflicting(const std::vector<const Action *> &jointAction) const;

//     inline bool is_cell_free(const Cell2D &cell) const;

//     std::string toString() const;

//     mutable size_t hash_ = 0;

//    private:
//     // Creates a new state from a parent state and a joint action performed in that state.
//     CBSState(const CBSState *parent, std::vector<const Action *> jointAction);

//     const int g_;  // Cost of reaching this state
//     // Store random device for shuffling
//     static std::random_device rd_;
//     static std::mt19937 g_rd_;
// };

// // Custom hash function for State pointers
// struct CBSStatePtrHash {
//     std::size_t operator()(const CBSState *state_ptr) const {
//         if (!state_ptr) {
//             return 0;
//         }
//         return state_ptr->getHash();
//     }
// };

// // Custom equality operator for State pointers
// struct CBSStatePtrEqual {
//     bool operator()(const CBSState *lhs_ptr, const CBSState *rhs_ptr) const {
//         // Handle null pointers
//         if (lhs_ptr == rhs_ptr) {  // Both null or same pointer
//             return true;
//         }
//         if (!lhs_ptr || !rhs_ptr) {  // One is null, the other isn't
//             return false;
//         }
//         // Dereference pointers and use the State's equality operator
//         return *lhs_ptr == *rhs_ptr;
//     }
// };

// // Insert StateMemoryPoolAllocator class definition here
// class CBSStateMemoryPoolAllocator {
//    private:
//     static constexpr size_t ESTIMATED_STATES_IN_POOL = 10'000'000;
//     static constexpr size_t STATE_SIZE_FOR_POOL = sizeof(CBSState);
//     static constexpr size_t POOL_SIZE_BYTES = ESTIMATED_STATES_IN_POOL * STATE_SIZE_FOR_POOL;

//     alignas(alignof(CBSState)) std::vector<u_int8_t> memory_pool_;
//     size_t next_free_offset_ = 0;

//     CBSStateMemoryPoolAllocator() {
//         try {
//             memory_pool_.resize(POOL_SIZE_BYTES);
//         } catch (const std::bad_alloc &e) {
//             fprintf(stderr, "CRITICAL ERROR: Failed to allocate memory pool of %zu bytes for StateMemoryPoolAllocator.\n",
//             POOL_SIZE_BYTES); fprintf(stderr, "sizeof(CBSState) is %zu bytes.\n", sizeof(CBSState)); throw;  // Re-throw, program likely
//             cannot continue
//         }
//         // fprintf(stderr, "INFO: StateMemoryPoolAllocator pool initialized. Capacity: %zu bytes for approx %zu states.\n",
//         //         memory_pool_.capacity(), memory_pool_.capacity() / (sizeof(State) > 0 ? sizeof(State) : 1));
//     }

//     CBSStateMemoryPoolAllocator(const CBSStateMemoryPoolAllocator &) = delete;
//     CBSStateMemoryPoolAllocator &operator=(const CBSStateMemoryPoolAllocator &) = delete;

//    public:
//     static CBSStateMemoryPoolAllocator &getInstance() {
//         static CBSStateMemoryPoolAllocator instance;
//         return instance;
//     }

//     // Allocate memory for one State object from the pool
//     void *allocate(std::size_t size) {
//         // Ensure size is padded to be a multiple of alignment for subsequent allocations
//         // This is a simplified approach; robust padding might be slightly more complex
//         // if size itself isn't already suitable.
//         // std::size_t aligned_size = (size + alignof(State) - 1) & ~(alignof(State) - 1);
//         // if you want to pad size itself
//         // but usually sizeof(T) is used directly

//         std::size_t current_offset_aligned = (next_free_offset_ + alignof(CBSState) - 1) & ~(alignof(CBSState) - 1);

//         if (current_offset_aligned + size > memory_pool_.size()) {  // Use original size for check after aligning start
//             fprintf(stderr, "CRITICAL ERROR: CBSStateMemoryPoolAllocator pool exhausted...\n");
//             throw std::bad_alloc();
//         }
//         void *ptr = memory_pool_.data() + current_offset_aligned;
//         next_free_offset_ = current_offset_aligned + size;  // Use original size for increment
//         return ptr;
//     }

//     // @todo: Currently no-op, but could be used to return memory to the pool if needed.
//     void deallocate(void *ptr) noexcept {
//         (void)ptr;  // Suppress unused parameter warning
//     }

//     // Optional: Reset for multiple runs (if your main loop runs search multiple times)
//     void reset() {
//         fprintf(stderr, "INFO: CBSStateAllocator pool reset.\n");
//         next_free_offset_ = 0;
//         // For debugging, you might want to fill with a pattern:
//         // std::fill(memory_pool_.begin(), memory_pool_.end(), 0xCD);
//     }

//     inline size_t getUsedBytes() const { return next_free_offset_; }
//     inline size_t getTotalBytes() const { return memory_pool_.capacity(); }
//     inline size_t getRemainingBytes() const { return memory_pool_.capacity() - next_free_offset_; }
// };

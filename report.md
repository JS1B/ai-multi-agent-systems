Artificial Intelligence and Multi-Agent Systems
===============================================

The course was taken in spring 2025.
Throughout the course I was a part of a 3 person team, but we parted ways before submition of the final project. Although regrettable, I still managed to complete the project on my own, in a way I inteded to. Some of the topics in the report may therefore be shared, mainly the initial part of this report, specifically:

- some of the assumptions and analysis
- c++ codebase that we built on top of individually

Introduction and Problem Context
--------------------------------

- **1.1. Project Goal:** Development of an advanced multi-agent client for the MAvis server within the hospital domain.
- **1.2. Competition Overview:** Brief explanation of the competition structure, scoring mechanisms (action cost, logarithmic time cost), and performance objectives (solving levels within 3 min/20,000 actions, aiming for high scores).
- **1.3. Solution Focus:** Justification for selecting Conflict-Based Search (CBS) as the core algorithmic approach for multi-agent pathfinding, emphasizing its suitability for the given problem constraints.

I struggled immensily with the project, I rewrote the whole codebase from scratch, albeit with some help of my previous teammates. I went back and forth with how I want to architect the project and how I want to structure the code. Possibly the biggest challenge was to wrap my hear around how a cbs with meta agents could be implemented for the hospital domain. It was bearable to implement the classic cbs with simple moves and waits, anything beyond that was a challenge.

Core Algorithmic Approach: Conflict-Based Search (CBS)
------------------------------------------------------

- **2.1. CBS Fundamentals:** A concise explanation of the basic CBS algorithm, including:
  - High-Level Search (Conflict Tree)
  - Low-Level Search (Single-Agent Pathfinding)
  - Conflict detection and resolution strategies (vertex, trailing).
    - edge conflicts were implemented but removed as i realized trailing conflicts covers it
- **2.2. Adaptation for MAvis Domain:** Specific considerations for applying CBS to the MAvis hospital environment, addressing unique aspects of the domain (e.g., box handling, specific agent actions).

Optimizations and Enhancements
------------------------------

**3.1. Algorithmic optimizations:**

- conflict detection and resolution strategies (vertex, edge conflicts).
- architecture design - separate modules for different parts of the algorithm
- low level search - A* with modified custom heuristic
- high level, cbs search - custom cost function to prioritize lowering total actions taken, then using sum of costs. Although fuel was also considered as an agent that stays and waits for its turn looks classy, instead of running back and forth.
- state shuffling - done in warmup, not done here
- use of meta agents idea but classic implementation was problematic for the hosptial domain - agents of the same color start merged
- cbs nodes duplication detection - using a set of detected one-sided conflicts to detect duplicates
- boxes with no agents were added as walls to avoid cbs handling conflicts with them - performance improvement

**3.2. Language-specific optimizations:**

- language selection - C++ for performance, readability and scalability and frankly to get to know it more
- memory management
  - memory pools allocator with new and delete operators overloading - analyzed and implemented but abandoned as did not provide any performance benefits
  - memory allocators were researched, specifically custom one for state due to the fact that each small allocation is essentially a syscall and thus slow. The idea was to allocate a large block of memory and then use it for all states. This was replaced with a simple .reserve method everywhere possible (vectors, sets, strings).
  - using references and read-only restictions whenever possible to prevent copying and increase performance
  - could be further imploved by using `move` semantics instead of copying
- caching
  - for expanded states generation
  - for hashes
- parallelization - not done - could be done by running multiple CBS instances in parallel, reference paper with node forest not just one conflict tree
- hashing and equality checking modifications
  - to differentiate between states I only used the difference in agents and boxes and nothing else. When constraints were added, depth was also considered but not before.
- early exits for:
  - isApplicable
  - isConflicting
  - getExpandedStates - due to caching
- encapsulation of static parts of the state (and the level) - structure with O(1) lookups, saves memory as its read only. Walls - 2D vector of chars, colors - map of chars (symbols) to colors
- encapsulation of dynamic parts of the state (agents and boxes) - structures with x, y coordinates, slower for lookups and interactions but saves memory on big levels and time allocating it when copying
- using maps and sets instead of vectors and arrays when it made sense
- usage of usage of `const` and `constexpr` whenever possible - to either restrict a variable to be read-only therefore faster or to make calculations at compile time
- usage of fast types, like `uint_fast8_t` instead of `uint8_t`

Conclusion and Future Work
--------------------------

- **6.1. Summary of Contributions:** Recap of the project's key achievements and the effectiveness of the CBS approach with custom optimizations.
- **6.2. Future Directions:** Potential improvements and extensions for the client, such as:
  - Advanced conflict resolution (e.g., using MACRO actions).
  - Improved low-level heuristics.
  - Further exploration of multi-agent coordination paradigms.
  - Exploration of alternative domains.

Appendices
----------

- Code snippets for critical sections (if appropriate).
- Detailed log excerpts from specific runs.
- Visualizations of challenging level solutions (if possible).

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

Core Algorithmic Approach: Conflict-Based Search (CBS)
------------------------------------------------------

- **2.1. CBS Fundamentals:** A concise explanation of the basic CBS algorithm, including:
  - High-Level Search (Conflict Tree)
  - Low-Level Search (Single-Agent Pathfinding)
  - Conflict detection and resolution strategies (vertex, edge conflicts).
- **2.2. Adaptation for MAvis Domain:** Specific considerations for applying CBS to the MAvis hospital environment, addressing unique aspects of the domain (e.g., box handling, specific agent actions).

Optimizations and Enhancements
------------------------------

- **3.1. Memory Management Strategies:** Detailed description of memory-related optimizations implemented to enhance CBS performance and scalability. This should cover:
  - Techniques used (e.g., state compression, intelligent data structures, garbage collection or reference counting approaches, hash-based caching).
  - Motivation behind these choices, especially given the rewrite to C++ and large state spaces.
- **3.2. Performance-Driven Improvements:** Other key optimizations applied to the CBS framework, potentially including:
  - Heuristics for low-level search (e.g., A* variants with specific cost functions).
  - Conflict selection and prioritization.
  - Potential for path-maxing or other search-space pruning techniques.
  - Any parallelization or concurrency considerations.

C++ Implementation Details
--------------------------

- **4.1. Architectural Design:** Overview of the C++ codebase structure, outlining key modules, classes, and their interactions (e.g., `Agent` class, `Level` representation, `SearchNode`, `ConflictTree`).
- **4.2. Implementation Challenges:** Discuss specific technical hurdles encountered during the C++ rewrite and solo development, and how they were addressed.
- **4.3. Comparison to Warmup / Reference Client:** Highlight significant differences and improvements over the initial Java warmup assignment client or any baseline.

Competition Performance Analysis
--------------------------------

- **5.1. Scoring Strategy:** How the client was tuned to balance action and time scores across various levels, considering the logarithmic nature of the time score.
- **5.2. Results Interpretation:** Analysis of the client's performance on the competition levels, using data from the provided scoreboards. This section should address:
  - Strengths: Levels or types of problems where the client excelled.
  - Weaknesses: Levels or scenarios where the client struggled (e.g., timeout, action limit).
  - Impact of optimizations on practical performance.
- **5.3. Case Studies (Optional but Recommended):** In-depth analysis of specific, challenging levels to demonstrate algorithmic behavior and effectiveness.

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

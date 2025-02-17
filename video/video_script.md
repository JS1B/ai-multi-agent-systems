# Video Script

## Video Plan

| Exercise | Time |
| -------- | ---- |
| 1        | 1min |
| 2        | 2min |
| 3        | 2min |
| 4        | 2min |
| 5        | 2min |
| 6        | 2min |
| 7        | 1min |


## Exercise 1
- `SearchClient` is entry point of the application and that is where `main()` function is. It parses arguments, constructs initial state, chooses `Frontier`, `Heuristic` and h (`CustomH` added by us) function to be used in `GraphSearch`.
- `GraphSearch` is the main logic of a solution finding algorithm. It uses `Frontier` to manage the states as well as functions from `State` class to generate child nodes and check if goal was reached.
- `Frontier` file defines 4 strategies in which the search should be ordered. Currently implemented strategies are BFS, DFS, AStar, Greedy. Strategies Astar and Greedy use the function h.
- `CustomH` file contains several implementations of h functions: Zero, GoalCount, BoxGoalCount, Custom, BoxCustom.
- `State` class holds the current state with common features of the states defined as `static` fields to avoid unnecessary computational overhead. Furthermore `State` class implements useful functions like: `isGoalState()`, `extractPlan()`, `getExpandedStates()`, `isApplicable()`, `isConflicting()`. `State` constructor with `isApplicable()`, `isConflicting()` functions were later expanded by us to work for levels with multiple agents and boxes.
- Files `Action` and  `Color` implement enums that make code better organized and more readable. They are used in mostly in a `State` class.
- Files `Memory` and `NotImplementedException` are files defining utilities and are not connected in any way to the search algorithm.

## Exercise 2

### Exercise 2.1
- DFS finds solution much faster but produces worse solutions that require significantly much more steps to solve the level. This stems from the fact that DFS builds on the current solution as long as it can, regardless of solution length, and returns immediately when it finds one. BFS on the other hand searches potential solution in order of solution length.
- DFS is implemented as stack (LIFO), the code is exactly the same as provided code for FrontierBFS with `add` function changed to use 
`queue.addFirst` instead of `queue.addLast`, and this change transforms queue into stack.

### Exercise 2.2
- BFS finds a solution only expanding 1 state, as it expands first states, that are nearest and in this way it completely avoids searching through vast comb-like structure. DFS on the other hand expands current solution as long as it can, which means that if it is "unlucky" it can start expanding in the opposite direction and miss the goal completely. BFSFriendly shows exactly that - DFS first searches for goal in wrong direction searching through comb-like structure and only after it cannot expand in comb structure anymore it searches in the correct direction and finds a solution. It is worth noting that with enough luck DFS can find optimal solution immediately, but on average it will be finding bad solutions compared to the optimal one.

## Exercise 3


## Exercise 4

### Exercise 4.3
1. Precompute the sum of distances od each agent to its goal.
2. It is -1 when goal unreachable, and is invalid when goal is in a wall
3. The heuristic is a sum of all distances. In the case of a state being unreachable, big fixed value gets specified (to stop searching derivatives of not needed heuristic)

* Strengths: generally doing better than defaults, precomputes for performance 
* Weaknesses: not taking boxes into account, uses bfs, so not always optimal

## Exercise 5

### Exercise 5.1
[Mention and show the implementation of push and pull actions]

While moving and pushing the box north, the box can only go north, east or west:
- Push(N,N)
- Push(N,E)
- Push(N,W)
etc

The box can be pushed in any direction, but the agent can only move in the direction of the box.
It is important to perform the legal actions only, otherwise the server will reject them.

### Exercise 5.2
[Show the benchmark results table]
[Play the videos of the 3 levels]

The difference in the complexity of solutions found by BFS for the 3 levels is significant. This is a result of how BFS explores the search space and the shape of the levels. Here:

- SAD1 - 1 box, 1 box goal
- SAD2 - 3 boxes, 1 box goal
- SAD3 - 6 boxes, 1 box goal

### Exercise 5.3
[Show the benchmark results table]
[Play the videos of the 2 levels]

The difference between the solutions' lengths found by DFS and BFS is extreme. Again, this is a result of how BFS and DFS explore the search space and the fact that it does it for each agent and box.
The reported time to solve, however, is not significantly different. This is due to the fact the measured time is impacted by the non-instant client and server communication, and the many moves DFS performs. 

## Exercise 6

### Exercise 6.2
The only difference compared to heuristic from 4.3, is that heuristic also takes into account boxes distance from their goal

* Strengths: the same as above
* Weaknesses: the same as above, but does take boxes into account

## Exercise 7

[Show the benchmark results table]

We created a few custom heuristics:

- GoalCount - in code "HGoalCount"
- BoxGoalCount - in code "HBoxGoalCount"
- Custom - in code "HSumDistances"
- BoxCustom - in code "HSumDistancesBox"
- BoxCustom2 - in code "HSumDistancesBox2"

All of them are implemented to work with both multiple agents and boxes.
We played with the parameters of the heuristics to see how they perform and designed 3 custom levels to test them.

We realized that there may not be a single simple heuristic that would perform best for all the levels. It depends on the level geometry and the placement and number of the boxes and goals. However, we noticed that BoxCustom2 generally performs the best, as it is both fast computationally and finds solutions with lower number of moves. It works by prioritizing pushing the boxes to the goals, and once done, it prioritizes moving the agents to the goals.

We also found that the geometry of the MAcustom03 level defeats almost all the heuristics. Therefore there is still a lot of room for improvement.

[Play the videos of the MAcustom01 boxcustom2, boxgoalcount and MAcustom02 boxcustom2, boxgoalcount]

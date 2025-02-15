Benchmark table for uninformed search

| Level             | Strategy | States Generated | Time to solve [s] | Solution Length |
| ----------------- | -------- | ---------------- | ----------------- | --------------- |
| MAPF00            | BFS      | 48               | 0.052             | 14              |
| MAPF00            | DFS      | 41               | 0.073             | 18              |
| MAPF01            | BFS      | 2350             | 0.170             | 14              |
| MAPF01            | DFS      | 1270             | 0.098             | 147             |
| MAPF02            | BFS      | 110445           | 6.638             | 14              |
| MAPF02            | DFS      | 8218             | 0.113             | 207             |
| MAPF02C           | BFS      | 110540           | 8.095             | 14              |
| MAPF02C           | DFS      | 207              | 1.532             | 3538            |
| MAPF03            | BFS      | 5063873          | 1937.427          | 14              |
| MAPF03            | DFS      | 128511           | 0.646             | 608             |
| MAPF03C           | BFS      | 5084159          | 1989.211          | 14              |
| MAPF03C           | DFS      | 4230326          | 37.628            | 52998           |
| MAPFslidingpuzzle | BFS      | 181289           | 2.144             | 28              |
| MAPFslidingpuzzle | DFS      | 163454           | 5.127             | 57558           |
| MAPFreorder2      | BFS      | 3603599          | 228.271           | 38              |
| MAPFreorder2      | DFS      | 3595945          | 62.780            | 78862           |
| BFSfriendly       | BFS      | 2                | 0.027             | 1               |
| BFSfriendly       | DFS      | 1003             | 0.070             | 1               |


Benchmark table for informed search

| Level             | Eval   | Heuristic  | States Generated | Time to solve [s] | Solution length |
| ----------------- | ------ | ---------- | ---------------- | ----------------- | --------------- |
| MAPF00            | A*     | Goal Count | 48               | 0.032             | 14              |
| MAPF00            | Greedy | Goal Count | 45               | 0.028             | 16              |
| MAPF01            | A*     | Goal Count | 2311             | 0.093             | 14              |
| MAPF01            | Greedy | Goal Count | 1771             | 0.070             | 137             |
| MAPF02            | A*     | Goal Count | 108206           | 5.726             | 14              |
| MAPF02            | Greedy | Goal Count | 18451            | 0.135             | 206             |
| MAPF02C           | A*     | Goal Count | 106051           | 6.673             | 14              |
| MAPF02C           | Greedy | Goal Count | 5168             | 0.108             | 44              |
| MAPF03            | A*     | Goal Count | -                | -                 | -               |
| MAPF03            | Greedy | Goal Count | 156727           | 0.578             | 364             |
| MAPF03C           | A*     | Goal Count | -                | -                 | -               |
| MAPF03C           | Greedy | Goal Count | 129608           | 2.362             | 55              |
| MAPFslidingpuzzle | A*     | Goal Count | 104391           | 1.183             | 28              |
| MAPFslidingpuzzle | Greedy | Goal Count | 962              | 0.077             | 46              |
| MAPFreorder2      | A*     | Goal Count | -                | -                 | -               |
| MAPFreorder2      | Greedy | Goal Count | -                | -                 | -               |
| BFSfriendly       | A*     | Goal Count | 2                | 0.049             | 1               |
| BFSfriendly       | Greedy | Goal Count | 2                | 0.054             | 1               |



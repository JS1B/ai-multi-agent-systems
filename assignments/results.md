Benchmark table for uninformed search - exc 3

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


Benchmark table for informed search - exc 4.2

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
| MAPF03            | A*     | Goal Count | 4755791          | 2176.890          | 14              |
| MAPF03            | Greedy | Goal Count | 156727           | 0.578             | 364             |
| MAPF03C           | A*     | Goal Count | 4755791          | 2543.181          | 14              |
| MAPF03C           | Greedy | Goal Count | 129608           | 2.362             | 55              |
| MAPFslidingpuzzle | A*     | Goal Count | 104391           | 1.183             | 28              |
| MAPFslidingpuzzle | Greedy | Goal Count | 962              | 0.077             | 46              |
| MAPFreorder2      | A*     | Goal Count | 3600155          | 224.614           | 40              |
| MAPFreorder2      | Greedy | Goal Count | 1583879          | 47.398            | 389             |
| BFSfriendly       | A*     | Goal Count | 2                | 0.049             | 1               |
| BFSfriendly       | Greedy | Goal Count | 2                | 0.054             | 1               |


Benchmark table for informed search - exc 4.3

| Level             | Eval   | Heuristic | States Generated | Time to solve [s] | Solution length |
| ----------------- | ------ | --------- | ---------------- | ----------------- | --------------- |
| MAPF00            | A*     | Custom    | 37               | 0.043             | 14              |
| MAPF00            | Greedy | Custom    | 29               | 0.031             | 14              |
| MAPF01            | A*     | Custom    | 742              | 0.054             | 14              |
| MAPF01            | Greedy | Custom    | 187              | 0.045             | 14              |
| MAPF02            | A*     | Custom    | 11035            | 0.149             | 14              |
| MAPF02            | Greedy | Custom    | 988              | 0.046             | 14              |
| MAPF02C           | A*     | Custom    | 723              | 0.052             | 15              |
| MAPF02C           | Greedy | Custom    | 723              | 0.046             | 15              |
| MAPF03            | A*     | Custom    | 135894           | 1.836             | 14              |
| MAPF03            | Greedy | Custom    | 3561             | 0.056             | 14              |
| MAPF03C           | A*     | Custom    | 2995             | 0.059             | 16              |
| MAPF03C           | Greedy | Custom    | 2995             | 0.061             | 16              |
| MAPFslidingpuzzle | A*     | Custom    | 3190             | 0.091             | 28              |
| MAPFslidingpuzzle | Greedy | Custom    | 311              | 0.059             | 58              |
| MAPFreorder2      | A*     | Custom    | 1945739          | 97.982            | 51              |
| MAPFreorder2      | Greedy | Custom    | 1813567          | 109.175           | 175             |
| BFSfriendly       | A*     | Custom    | 2                | 0.035             | 1               |
| BFSfriendly       | Greedy | Custom    | 2                | 0.037             | 1               |


Benchmark table for custom function - exc 5.2

| Level | Strategy | States Generated | Time to solve [s] | Solution length |
| ----- | -------- | ---------------- | ----------------- | --------------- |
| SAD1  | BFS      | 80               | 0.036             | 19              |
| SAD1  | DFS      | 75               | 0.035             | 27              |
| SAD2  | BFS      | 635264           | 2.505             | 19              |
| SAD2  | DFS      | 144676           | 1.873             | 41179           |
| SAD3  | BFS      | 10170000         | 61.744            | 19              |
| SAD3  | DFS      | 110              | 0.051             | 39              |


Benchmark table for custom function - exc 5.3

| Level     | Strategy | States Generated | Time to solve [s] | Solution length |
| --------- | -------- | ---------------- | ----------------- | --------------- |
| SAFirefly | BFS      | 1920105          | 10.306            | 60              |
| SAFirefly | DFS      | 2321546          | 7.564             | 1474392         |
| SACrunch  | BFS      | 9284096          | 47.605            | 98              |
| SACrunch  | DFS      | 3380266          | 37.813            | 1110820         |


Benchmark table for custom function - exc 6.2

| Level             | Eval   | Heuristic  | States Generated | Time to solve [s] | Solution length |
| ----------------- | ------ | ---------- | ---------------- | ----------------- | --------------- |
| MAPF00            | A*     | Box Custom | 37               | 0.051             | 14              |
| MAPF00            | Greedy | Box Custom | 29               | 0.048             | 14              |
| MAPF01            | A*     | Box Custom | 742              | 0.066             | 14              |
| MAPF01            | Greedy | Box Custom | 187              | 0.059             | 14              |
| MAPF02            | A*     | Box Custom | 11035            | 0.168             | 14              |
| MAPF02            | Greedy | Box Custom | 988              | 0.060             | 14              |
| MAPF02C           | A*     | Box Custom | 723              | 0.040             | 15              |
| MAPF02C           | Greedy | Box Custom | 723              | 0.059             | 15              |
| MAPF03            | A*     | Box Custom | 135894           | 1.071             | 14              |
| MAPF03            | Greedy | Box Custom | 3561             | 0.055             | 14              |
| MAPF03C           | A*     | Box Custom | 2995             | 0.070             | 16              |
| MAPF03C           | Greedy | Box Custom | 2995             | 0.052             | 16              |
| MAPFslidingpuzzle | A*     | Box Custom | 3190             | 0.094             | 28              |
| MAPFslidingpuzzle | Greedy | Box Custom | 311              | 0.068             | 58              |
| MAPFreorder2      | A*     | Box Custom | 1945739          | 58.986            | 51              |
| MAPFreorder2      | Greedy | Box Custom | 1813567          | 58.555            | 175             |
| BFSfriendly       | A*     | Box Custom | 2                | 0.027             | 1               |
| BFSfriendly       | Greedy | Box Custom | 2                | 0.031             | 1               |


Benchmark table for extension - exc 7

| Level      | Eval   | Heuristic      | States Generated | Time to solve [s] | Solution length |
| ---------- | ------ | -------------- | ---------------- | ----------------- | --------------- |
| MAPF03C    | Greedy | Goal Count     | 129608           | 2.309             | 55              |
| MAPF03C    | Greedy | Box Goal Count | 129608           | 2.731             | 55              |
| MAPF03C    | Greedy | Custom         | 2995             | 0.074             | 16              |
| MAPF03C    | Greedy | Box Custom     | 2995             | 0.083             | 16              |
| MAPF03C    | Greedy | Box Custom 2   | 2995             | 0.081             | 16              |
| MAPF03C    | A*     | Goal Count     | -                | >1000             | -               |
| MAPF03C    | A*     | Box Goal Count | -                | >1000             | -               |
| MAPF03C    | A*     | Custom         | 2995             | 0.070             | 16              |
| MAPF03C    | A*     | Box Custom     | 2995             | 0.081             | 16              |
| MAPF03C    | A*     | Box Custom 2   | 2995             | 0.073             | 16              |
| MAcustom01 | Greedy | Goal Count     | 40185            | 0.378             | 87              |
| MAcustom01 | Greedy | Box Goal Count | 4388             | 0.138             | 50              |
| MAcustom01 | Greedy | Custom         | 162474           | 2.066             | 45              |
| MACustom01 | Greedy | Box Custom     | 3202             | 0.106             | 94              |
| MAcustom01 | Greedy | Box Custom 2   | 437              | 0.058             | 20              |
| MAcustom01 | A*     | Goal Count     | 255021           | 3.989             | 20              |
| MAcustom01 | A*     | Box Goal Count | 254991           | 4.716             | 20              |
| MAcustom01 | A*     | Custom         | 250213           | 4.144             | 25              |
| MAcustom01 | A*     | Box Custom     | 107886           | 1.489             | 24              |
| MAcustom01 | A*     | Box Custom 2   | 437              | 0.051             | 20              |
| MAcustom02 | Greedy | Goal Count     | 3560857          | 16.659            | 129             |
| MAcustom02 | Greedy | Box Goal Count | 7402             | 0.168             | 60              |
| MAcustom02 | Greedy | Custom         | 13162311         | 170.299           | 314             |
| MAcustom02 | Greedy | Box Custom     | 407394           | 4.388             | 167             |
| MAcustom02 | Greedy | Box Custom 2   | 155267           | 2.086             | 122             |
| MAcustom02 | A*     | Goal Count     | -                | >1000             | -               |
| MAcustom02 | A*     | Box Goal Count | -                | >1000             | -               |
| MAcustom02 | A*     | Custom         | -                | >1000             | -               |
| MAcustom02 | A*     | Box Custom     | 11991448         | 201.638           | 30              |
| MAcustom02 | A*     | Box Custom 2   | 807235           | 11.082            | 37              |
| MAcustom03 | Greedy | Goal Count     | -                | >1000             | -               |
| MAcustom03 | Greedy | Box Goal Count | 24830            | 0.308             | 74              |
| MAcustom03 | Greedy | Custom         | -                | >1000             | -               |
| MAcustom03 | Greedy | Box Custom     | -                | >1000             | -               |
| MAcustom03 | Greedy | Box Custom 2   | -                | >1000             | -               |
| MAcustom03 | A*     | Goal Count     | -                | >1000             | -               |
| MAcustom03 | A*     | Box Goal Count | -                | >1000             | -               |
| MAcustom03 | A*     | Custom         | -                | >1000             | -               |
| MAcustom03 | A*     | Box Custom     | -                | >1000             | -               |
| MAcustom03 | A*     | Box Custom 2   | -                | >1000             | -               |

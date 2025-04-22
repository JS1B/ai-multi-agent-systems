# Searchclient cpp

It describes how to use the included C++ searchclient with the server that is contained in server.jar.

The search client requires a C++17 compatible compiler (g++).

All the following commands assume the working directory is the one this readme is located in.

You can read about the server options using the -h argument:

```
java -jar ../server.jar -h
```

Compiling the searchclient:
```
make
```

Starting the server using the searchclient:

```
java -jar ../server.jar -l ../levels/SAD1.lvl -c "./searchclient" -g -s 150 -t 180
```

The searchclient uses the BFS strategy by default. Use arguments -dfs, -astar, -wastar, or -greedy to set alternative search strategies (after you implement them). For instance, to use DFS on the same level as above:

```
java -jar ../server.jar -l ../levels/SAD1.lvl -c "./searchclient -dfs" -g -s 150 -t 180
```
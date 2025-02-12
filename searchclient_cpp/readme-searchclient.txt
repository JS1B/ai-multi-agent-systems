/*******************************************************\
|                     Searchclient                      |
|                        README                         |
\*******************************************************/

This readme describes how to use the included C++ searchclient with the server that is contained in server.jar.

The search client requires a C++17 compatible compiler (g++).

All the following commands assume the working directory is the one this readme is located in.

You can read about the server options using the -h argument:
    $ java -jar ../server.jar -h

Compiling the searchclient:
    $ make

Starting the server using the searchclient:
    $ java -jar ../server.jar -l ../levels/SAD1.lvl -c "./searchclient" -g -s 150 -t 180

The searchclient uses the BFS strategy by default. Use arguments -dfs, -astar, -wastar, or -greedy to set alternative search strategies (after you implement them). For instance, to use DFS on the same level as above:
    $ java -jar ../server.jar -l ../levels/SAD1.lvl -c "./searchclient -dfs" -g -s 150 -t 180

Memory settings:
    * Unless your hardware is unable to support this, you should let the process allocate at least 4GB of memory *
    The -Xmx option sets the maximum size of the heap, i.e. how much memory your program can allocate.
    The -Xms option sets the initial size of the heap.
    To set the max heap size to 4GB:
        $ java -jar ../server.jar -l ../levels/SAD1.lvl -c "./searchclient" -g -s 150 -t 180
    Note that this option is set for the *client*.
    Avoid setting max heap size too high, since it will lead to your OS doing memory swapping which is terribly slow. 
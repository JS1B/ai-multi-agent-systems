package searchclient;

import java.util.HashSet;

public class GraphSearch {

    public static Action[][] search(State initialState, Frontier frontier)
    {
        boolean outputFixedSolution = false;

        if (outputFixedSolution) {
            //Part 1:
            //The agents will perform the sequence of actions returned by this method.
            //Try to solve a few levels by hand, enter the found solutions below, and run them:

            return new Action[][] {
                {Action.MoveS},
                {Action.MoveS},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveS},
                {Action.MoveS},
            };

            /* 
            return new Action[][] {
                {Action.MoveS},
                {Action.MoveE},
                {Action.MoveE},
                {Action.MoveS},
            };
            */
        } else {
            //Part 2:
            //Now try to implement the Graph-Search algorithm from R&N figure 3.7
            //In the case of "failure to find a solution" you should return null.
            //Some useful methods on the state class which you will need to use are:
            //state.isGoalState() - Returns true if the state is a goal state.
            //state.extractPlan() - Returns the Array of actions used to reach this state.
            //state.getExpandedStates() - Returns an ArrayList<State> containing the states reachable from the current state.
            //You should also take a look at Frontier.java to see which methods the Frontier interface exposes
            //
            //printSearchStatus(expanded, frontier): As you can see below, the code will print out status
            //(#expanded states, size of the frontier, #generated states, total time used) for every 10000th node generated.
            //You should also make sure to print out these stats when a solution has been found, so you can keep
            //track of the exact total number of states generated!!


            int iterations = 0;

            frontier.add(initialState);
            HashSet<State> expanded = new HashSet<>();

            while (true) {
                if (frontier.isEmpty()) {
                    printSearchStatus(expanded, frontier);
                    return null;
                }

                State s = frontier.pop();

                if (s.isGoalState())
                {
                    printSearchStatus(expanded, frontier);
                    return s.extractPlan();
                }

                expanded.add(s);

                for (State t : s.getExpandedStates()) {
                    if (!frontier.contains(t) && !expanded.contains(t))
                        frontier.add(t);
                }

                //Print a status message every n iteration
                if (!SearchClient.benchmark_logs && ++iterations % 10000 == 0) {
                    printSearchStatus(expanded, frontier);
                }

                //Your code here... Don't forget to print out the stats when a solution has been found (see above)
            }
        }
    }

    private static long startTime = System.nanoTime();

    private static void printSearchStatus(HashSet<State> expanded, Frontier frontier)
    {
        String statusTemplate = "#Expanded: %,8d, #Frontier: %,8d, #Generated: %,8d, Time: %3.3f s\n%s\n";
        double elapsedTime = (System.nanoTime() - startTime) / 1_000_000_000d;
        System.err.format(statusTemplate, expanded.size(), frontier.size(), expanded.size() + frontier.size(),
                          elapsedTime, Memory.stringRep());
    }
}

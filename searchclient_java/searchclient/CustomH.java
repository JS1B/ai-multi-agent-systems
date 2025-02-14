package searchclient;

import java.util.ArrayList;
import java.util.AbstractMap.SimpleEntry;

public interface CustomH {
    void init(State initialState);  // Optional initialization
    int h(State s);
}


class HZero implements CustomH {
    @Override
    public void init(State initialState) 
    {
        // Here's a chance to pre-process the static parts of the level.
    }

    @Override
    public int h(State s)
    {
        return 0;
    }
    
    @Override
    public String toString()
    {
        return "HZero";
    }
}

class HGoalCount implements CustomH {
    // GoalCount heuristic for levels without boxes

    private ArrayList<SimpleEntry<Integer, Integer>> goalList = new ArrayList<>();

    @Override
    public void init(State initialState) 
    {
        int rows = State.goals.length;
        int cols = State.goals[0].length;

        if (!SearchClient.benchmark_logs) {
            System.err.format("Printing State.goals[%d][%d] array:\n", rows, cols);
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    System.err.print(State.goals[r][c]);  // Print each character
                }
                System.err.println();  // Newline after each row
            }
            System.err.println("------------------------------");
        }
        

        // It turns out that walls are black and are not distinguishible from background
        // e.g MAPFslidingpuzzle.lvl is (5,5) even though it looks like (3,3) because it has walls around the map

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {   
                if (State.goals[r][c] != 0 && State.goals[r][c] >= '0' && State.goals[r][c] <= '9') // IDK why but they use null (value 0) instead of space (value 32) as blank
                {
                    int g = State.goals[r][c];
                    //System.err.format("r = %d, c = %d, State.goals[r][c] = %c, g = %d\n", r, c, State.goals[r][c], g);
                    goalList.add(new SimpleEntry<>(r, c));
                }
            }
        }
    }

    @Override
    public int h(State s)
    {
        int sum = 0;
        for (SimpleEntry<Integer, Integer> g : goalList) {
            int r = g.getKey();
            int c = g.getValue();
            int agent = State.goals[r][c] - '0';
            if (s.agentRows[agent] != r || s.agentCols[agent] != c)
                sum++;
        }

        return sum;
    }

    @Override
    public String toString()
    {
        return "HGoalCount";
    }
}

class HBoxGoalCount implements CustomH {
    // GoalCount heuristic for levels with boxes

    private ArrayList<SimpleEntry<Integer, Integer>> agentGoalList = new ArrayList<>();
    private ArrayList<SimpleEntry<Integer, Integer>> boxGoalList = new ArrayList<>();

    @Override
    public void init(State initialState) 
    {
        int rows = State.goals.length;
        int cols = State.goals[0].length;

        if (!SearchClient.benchmark_logs) {
            System.err.format("Printing State.goals[%d][%d] array:\n", rows, cols);
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    System.err.print(State.goals[r][c]);  // Print each character
                }
                System.err.println();  // Newline after each row
            }
            System.err.println("------------------------------");
        }
        
        // It turns out that walls are black and are not distinguishible from background
        // e.g MAPFslidingpuzzle.lvl is (5,5) even though it looks like (3,3) because it has walls around the map

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {  
                if (State.goals[r][c] == 0) // IDK why but they use null (value 0) instead of space (value 32) as blank
                    continue;

                if (State.goals[r][c] >= '0' && State.goals[r][c] <= '9') {
                    agentGoalList.add(new SimpleEntry<>(r, c));
                }

                if (State.goals[r][c] >= 'A' && State.goals[r][c] <= 'Z') {
                    boxGoalList.add(new SimpleEntry<>(r, c));
                }
            }
        }
    }

    @Override
    public int h(State s)
    {
        int sum = 0;
        for (SimpleEntry<Integer, Integer> g : agentGoalList) {
            int r = g.getKey();
            int c = g.getValue();
            int agent = State.goals[r][c] - '0';
            if (s.agentRows[agent] != r || s.agentCols[agent] != c)
                sum++;
        }

        for (SimpleEntry<Integer, Integer> g : boxGoalList) {
            int r = g.getKey();
            int c = g.getValue();
            int expectedBox = State.goals[r][c];
            int box = s.boxes[r][c];
            if (box != expectedBox)
                sum++;
        }

        return sum;
    }

    @Override
    public String toString()
    {
        return "HBoxGoalCount";
    }
}
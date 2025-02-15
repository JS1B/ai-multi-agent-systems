package searchclient;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;
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
                    goalList.add(new SimpleEntry<>(r, c));
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

class HSumDistances implements CustomH {
    private Map<Integer, SimpleEntry<Integer, Integer>> agentGoals = new HashMap<>();
    private Map<Integer, int[][]> distanceMaps = new HashMap<>();

    @Override
    public void init(State initialState) {
        int numAgents = initialState.agentRows.length;
        int rows = State.goals.length;
        int cols = State.goals[0].length;

        // Map each agent to their goal position
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                char goalChar = (char) State.goals[r][c];
                if (goalChar >= '0' && goalChar <= '9') {
                    int agent = goalChar - '0';
                    if (agent < numAgents) {
                        agentGoals.put(agent, new SimpleEntry<>(r, c));
                    }
                }
            }
        }

        // Precompute BFS distances for each agent's goal
        for (Map.Entry<Integer, SimpleEntry<Integer, Integer>> entry : agentGoals.entrySet()) {
            int agent = entry.getKey();
            SimpleEntry<Integer, Integer> goal = entry.getValue();
            int[][] distances = computeBFSDistanceMap(goal.getKey(), goal.getValue());
            distanceMaps.put(agent, distances);
        }
    }

    // BFS to compute shortest path distances from a goal cell to all other cells
    private int[][] computeBFSDistanceMap(int goalRow, int goalCol) {
        int rows = State.walls.length;
        int cols = State.walls[0].length;
        int[][] distances = new int[rows][cols];
        for (int[] row : distances) Arrays.fill(row, -1); // -1 indicates unreachable

        if (State.walls[goalRow][goalCol]) return distances; // Goal is in a wall (invalid)

        Queue<SimpleEntry<Integer, Integer>> queue = new LinkedList<>();
        distances[goalRow][goalCol] = 0;
        queue.add(new SimpleEntry<>(goalRow, goalCol));

        int[][] directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

        while (!queue.isEmpty()) {
            SimpleEntry<Integer, Integer> cell = queue.poll();
            int r = cell.getKey();
            int c = cell.getValue();

            for (int[] dir : directions) {
                int newR = r + dir[0];
                int newC = c + dir[1];
                if (newR >= 0 && newR < rows && newC >= 0 && newC < cols) {
                    if (!State.walls[newR][newC] && distances[newR][newC] == -1) {
                        distances[newR][newC] = distances[r][c] + 1;
                        queue.add(new SimpleEntry<>(newR, newC));
                    }
                }
            }
        }

        return distances;
    }

    @Override
    public int h(State s) {
        int totalDistance = 0;
        for (Map.Entry<Integer, SimpleEntry<Integer, Integer>> entry : agentGoals.entrySet()) {
            int agent = entry.getKey();
            SimpleEntry<Integer, Integer> goal = entry.getValue();
            int currentRow = s.agentRows[agent];
            int currentCol = s.agentCols[agent];

            if (currentRow != goal.getKey() || currentCol != goal.getValue()) {
                int[][] distances = distanceMaps.get(agent);
                if (currentRow < 0 || currentRow >= distances.length || 
                    currentCol < 0 || currentCol >= distances[0].length) {
                    totalDistance += Integer.MAX_VALUE / 2; // Invalid position
                } else {
                    int distance = distances[currentRow][currentCol];
                    totalDistance += (distance == -1) ? Integer.MAX_VALUE / 2 : distance;
                }
            }
        }
        return totalDistance;
    }

    @Override
    public String toString()
    {
        return "HGoalCount";
    }
}

class HSumDistancesBox implements CustomH {
    private Map<Integer, SimpleEntry<Integer, Integer>> agentGoals = new HashMap<>();
    private Map<Integer, int[][]> distanceMaps = new HashMap<>();
    private Map<Character, SimpleEntry<Integer, Integer>> boxGoals = new HashMap<>();
    private Map<Character, int[][]> boxDistanceMaps = new HashMap<>();

    @Override
    public void init(State initialState) {
        int numAgents = initialState.agentRows.length;
        int rows = State.goals.length;
        int cols = State.goals[0].length;

        // Map each agent to their goal position
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                char goalChar = (char) State.goals[r][c];
                if (goalChar >= '0' && goalChar <= '9') {
                    int agent = goalChar - '0';
                    if (agent < numAgents) {
                        agentGoals.put(agent, new SimpleEntry<>(r, c));
                    }
                }
            }
        }

        // Precompute BFS distances for each agent's goal
        for (Map.Entry<Integer, SimpleEntry<Integer, Integer>> entry : agentGoals.entrySet()) {
            int agent = entry.getKey();
            SimpleEntry<Integer, Integer> goal = entry.getValue();
            int[][] distances = computeBFSDistanceMap(goal.getKey(), goal.getValue());
            distanceMaps.put(agent, distances);
        }

        // Map each box to its goal position
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                char goalChar = (char) State.goals[r][c];
                if (goalChar >= 'A' && goalChar <= 'Z') {
                    boxGoals.put(goalChar, new SimpleEntry<>(r, c));
                }
            }
        }

        // Precompute BFS distances for each box's goal
        for (Map.Entry<Character, SimpleEntry<Integer, Integer>> entry : boxGoals.entrySet()) {
            char boxChar = entry.getKey();
            SimpleEntry<Integer, Integer> goal = entry.getValue();
            int[][] distances = computeBFSDistanceMap(goal.getKey(), goal.getValue());
            boxDistanceMaps.put(boxChar, distances);
        }
    }

    // BFS to compute shortest path distances from a goal cell to all other cells
    private int[][] computeBFSDistanceMap(int goalRow, int goalCol) {
        int rows = State.walls.length;
        int cols = State.walls[0].length;
        int[][] distances = new int[rows][cols];
        for (int[] row : distances) Arrays.fill(row, -1); // -1 indicates unreachable

        if (State.walls[goalRow][goalCol]) return distances; // Goal is in a wall (invalid)

        Queue<SimpleEntry<Integer, Integer>> queue = new LinkedList<>();
        distances[goalRow][goalCol] = 0;
        queue.add(new SimpleEntry<>(goalRow, goalCol));

        int[][] directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

        while (!queue.isEmpty()) {
            SimpleEntry<Integer, Integer> cell = queue.poll();
            int r = cell.getKey();
            int c = cell.getValue();

            for (int[] dir : directions) {
                int newR = r + dir[0];
                int newC = c + dir[1];
                if (newR >= 0 && newR < rows && newC >= 0 && newC < cols) {
                    if (!State.walls[newR][newC] && distances[newR][newC] == -1) {
                        distances[newR][newC] = distances[r][c] + 1;
                        queue.add(new SimpleEntry<>(newR, newC));
                    }
                }
            }
        }

        return distances;
    }

    @Override
    public int h(State s) {
        int totalDistance = 0;

        // Sum agent distances
        for (Map.Entry<Integer, SimpleEntry<Integer, Integer>> entry : agentGoals.entrySet()) {
            int agent = entry.getKey();
            SimpleEntry<Integer, Integer> goal = entry.getValue();
            int currentRow = s.agentRows[agent];
            int currentCol = s.agentCols[agent];

            if (currentRow != goal.getKey() || currentCol != goal.getValue()) {
                int[][] distances = distanceMaps.get(agent);
                if (currentRow < 0 || currentRow >= distances.length || 
                    currentCol < 0 || currentCol >= distances[0].length) {
                    totalDistance += Integer.MAX_VALUE / 2; // Invalid position
                } else {
                    int distance = distances[currentRow][currentCol];
                    totalDistance += (distance == -1) ? Integer.MAX_VALUE / 2 : distance;
                }
            }
        }

        // Sum box distances
        for (Map.Entry<Character, SimpleEntry<Integer, Integer>> entry : boxGoals.entrySet()) {
            char boxChar = entry.getKey();
            SimpleEntry<Integer, Integer> goal = entry.getValue();
            int goalRow = goal.getKey();
            int goalCol = goal.getValue();

            boolean found = false;
            // Search for the current position of the box
            for (int r = 0; r < s.boxes.length; r++) {
                for (int c = 0; c < s.boxes[r].length; c++) {
                    if (s.boxes[r][c] == boxChar) {
                        int[][] distances = boxDistanceMaps.get(boxChar);
                        if (r < 0 || r >= distances.length || c < 0 || c >= distances[0].length) {
                            totalDistance += Integer.MAX_VALUE / 2;
                        } else {
                            int distance = distances[r][c];
                            totalDistance += (distance == -1) ? Integer.MAX_VALUE / 2 : distance;
                        }
                        found = true;
                        break; // Assuming each box character is unique
                    }
                }
                if (found) break;
            }
            if (!found) {
                totalDistance += Integer.MAX_VALUE / 2;
            }
        }

        return totalDistance;
    }

    @Override
    public String toString() {
        return "HSumDistancesBox";
    }
}
package searchclient;

import java.util.Comparator;

public abstract class Heuristic
        implements Comparator<State>
{
    // Choose h function here
    public static CustomH heur;

    public Heuristic(State initialState)
    {
        if (heur == null)
            System.err.println("Error: set heuristic with -heur parameter before setting frontier!");
        heur.init(initialState);
    }

    public int h(State s)
    {
        return heur.h(s);
    }

    public abstract int f(State s);

    @Override
    public int compare(State s1, State s2)
    {
        return this.f(s1) - this.f(s2);
    }

    public String hString()
    {
        return String.format("(h = %s)", heur.toString());
    }
}

class HeuristicAStar
        extends Heuristic
{
    public HeuristicAStar(State initialState)
    {
        super(initialState);
    }

    @Override
    public int f(State s)
    {
        return s.g() + this.h(s);
    }

    @Override
    public String toString()
    {
        return String.format("A* evaluation " + hString());
    }
}

class HeuristicWeightedAStar
        extends Heuristic
{
    private int w;

    public HeuristicWeightedAStar(State initialState, int w)
    {
        super(initialState);
        this.w = w;
    }

    @Override
    public int f(State s)
    {
        return s.g() + this.w * this.h(s);
    }

    @Override
    public String toString()
    {
        return String.format("WA*(%d) evaluation " + hString(), this.w);
    }
}

class HeuristicGreedy
        extends Heuristic
{
    public HeuristicGreedy(State initialState)
    {
        super(initialState);
    }

    @Override
    public int f(State s)
    {
        return this.h(s);
    }

    @Override
    public String toString()
    {
        return "greedy evaluation" + hString();
    }
}

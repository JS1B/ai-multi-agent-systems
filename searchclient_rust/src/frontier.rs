use std::cmp::Ordering;
use std::collections::{BinaryHeap, HashSet, VecDeque};

use crate::action::Action;
use crate::heuristic::{Heuristic, HeuristicAStar, HeuristicGreedy, HeuristicWeightedAStar};
use crate::state::State;

// Define an enum to hold the different heuristic types
pub enum HeuristicEnum<'a> {
    AStar(HeuristicAStar<'a>),
    WeightedAStar(HeuristicWeightedAStar<'a>),
    Greedy(HeuristicGreedy<'a>),
}

impl<'a> Heuristic for HeuristicEnum<'a> {
    fn h(&self, s: &State) -> isize {
        match self {
            HeuristicEnum::AStar(h) => h.h(s),
            HeuristicEnum::WeightedAStar(h) => h.h(s),
            HeuristicEnum::Greedy(h) => h.h(s),
        }
    }

    fn f(&self, s: &State) -> isize {
        match self {
            HeuristicEnum::AStar(h) => h.f(s),
            HeuristicEnum::WeightedAStar(h) => h.f(s),
            HeuristicEnum::Greedy(h) => h.f(s),
        }
    }
}

impl<'a> std::fmt::Display for HeuristicEnum<'a> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            HeuristicEnum::AStar(h) => write!(f, "{}", h),
            HeuristicEnum::WeightedAStar(h) => write!(f, "{}", h),
            HeuristicEnum::Greedy(h) => write!(f, "{}", h),
        }
    }
}

pub trait Frontier<'a> {
    fn add(&mut self, state: State<'a>);
    fn pop(&mut self) -> Option<State<'a>>;
    fn is_empty(&self) -> bool;
    fn size(&self) -> usize;
    fn contains(&self, state: &State<'a>) -> bool;
    fn get_name(&self) -> String;
    
    // Process a batch of states atomically
    fn process_batch(&mut self, state: State<'a>, expanded: &mut HashSet<u64>) -> Option<Vec<Vec<Action>>> {
        // Check if goal reached
        if state.is_goal_state() {
            return Some(state.extract_plan());
        }

        // Get expanded states and filter them
        let expanded_states = state.get_expanded_states();
        
        // Update expanded set
        expanded.insert(state.calculate_hash());
        
        // Process each expanded state
        for next_state in expanded_states {
            let hash = next_state.calculate_hash();
            if !expanded.contains(&hash) && !self.contains(&next_state) {
                self.add(next_state);
            }
        }
        
        None
    }
}

pub struct FrontierBFS<'a> {
    queue: VecDeque<State<'a>>,
    set: HashSet<u64>,
}

impl<'a> FrontierBFS<'a> {
    pub fn new() -> Self {
        FrontierBFS {
            queue: VecDeque::with_capacity(65536),
            set: HashSet::with_capacity(65536),
        }
    }
}

impl<'a> Frontier<'a> for FrontierBFS<'a> {
    fn add(&mut self, state: State<'a>) {
        let hash = state.calculate_hash();
        self.queue.push_back(state);
        self.set.insert(hash);
    }

    fn pop(&mut self) -> Option<State<'a>> {
        self.queue.pop_front().map(|state| {
            self.set.remove(&state.calculate_hash());
            state
        })
    }

    fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    fn size(&self) -> usize {
        self.queue.len()
    }

    fn contains(&self, state: &State<'a>) -> bool {
        self.set.contains(&state.calculate_hash())
    }

    fn get_name(&self) -> String {
        "breadth-first search".to_string()
    }
}

pub struct FrontierDFS<'a> {
    queue: VecDeque<State<'a>>,
    set: HashSet<u64>,
}

impl<'a> FrontierDFS<'a> {
    pub fn new() -> Self {
        FrontierDFS {
            queue: VecDeque::with_capacity(65536),
            set: HashSet::with_capacity(65536),
        }
    }
}

impl<'a> Frontier<'a> for FrontierDFS<'a> {
    fn add(&mut self, state: State<'a>) {
        let hash = state.calculate_hash();
        self.queue.push_front(state);
        self.set.insert(hash);
    }

    fn pop(&mut self) -> Option<State<'a>> {
        self.queue.pop_front().map(|state| {
            self.set.remove(&state.calculate_hash());
            state
        })
    }

    fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    fn size(&self) -> usize {
        self.queue.len()
    }

    fn contains(&self, state: &State<'a>) -> bool {
        self.set.contains(&state.calculate_hash())
    }

    fn get_name(&self) -> String {
        "depth-first search".to_string()
    }
}

// Wrapper for State to implement Ord for BinaryHeap
struct StateWithPriority<'a> {
    state: State<'a>,
    priority: isize,
}

impl<'a> PartialEq for StateWithPriority<'a> {
    fn eq(&self, other: &Self) -> bool {
        self.priority == other.priority
    }
}

impl<'a> Eq for StateWithPriority<'a> {}

impl<'a> PartialOrd for StateWithPriority<'a> {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl<'a> Ord for StateWithPriority<'a> {
    fn cmp(&self, other: &Self) -> Ordering {
        // Reverse ordering for min-heap
        other.priority.cmp(&self.priority)
    }
}

pub struct FrontierBestFirst<'a> {
    heuristic: HeuristicEnum<'a>,
    queue: BinaryHeap<StateWithPriority<'a>>,
    set: HashSet<u64>,
}

impl<'a> FrontierBestFirst<'a> {
    pub fn new(heuristic: HeuristicEnum<'a>) -> Self {
        FrontierBestFirst {
            heuristic,
            queue: BinaryHeap::new(),
            set: HashSet::with_capacity(65536),
        }
    }
}

impl<'a> Frontier<'a> for FrontierBestFirst<'a> {
    fn add(&mut self, state: State<'a>) {
        let hash = state.calculate_hash();
        let priority = self.heuristic.f(&state);
        self.queue.push(StateWithPriority {
            state,
            priority,
        });
        self.set.insert(hash);
    }

    fn pop(&mut self) -> Option<State<'a>> {
        self.queue.pop().map(|state_with_priority| {
            self.set.remove(&state_with_priority.state.calculate_hash());
            state_with_priority.state
        })
    }

    fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    fn size(&self) -> usize {
        self.queue.len()
    }

    fn contains(&self, state: &State<'a>) -> bool {
        self.set.contains(&state.calculate_hash())
    }

    fn get_name(&self) -> String {
        format!("best-first search using {}", self.heuristic.to_string())
    }
}

pub struct FrontierOps<'a> {
    frontier: &'a mut Box<dyn Frontier<'a>>,
}

impl<'a> FrontierOps<'a> {
    pub fn new(frontier: &'a mut Box<dyn Frontier<'a>>) -> Self {
        Self { frontier }
    }

    pub fn check_state(&mut self, state: &State<'a>) -> bool {
        !self.frontier.contains(state)
    }

    pub fn add_state(&mut self, state: State<'a>) {
        self.frontier.add(state);
    }

    pub fn get_next_state(&mut self) -> Option<(State<'a>, usize)> {
        if self.frontier.is_empty() {
            return None;
        }
        let size = self.frontier.size();
        self.frontier.pop().map(|state| (state, size))
    }
}

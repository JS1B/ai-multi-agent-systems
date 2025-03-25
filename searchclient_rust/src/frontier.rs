use std::cmp::Ordering;
use std::collections::{BinaryHeap, HashSet, VecDeque};

use crate::heuristic::{Heuristic, HeuristicAStar, HeuristicGreedy, HeuristicWeightedAStar};
use crate::state::State;

// Define an enum to hold the different heuristic types
pub enum HeuristicEnum {
    AStar(HeuristicAStar),
    WeightedAStar(HeuristicWeightedAStar),
    Greedy(HeuristicGreedy),
}

impl Heuristic for HeuristicEnum {
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

impl std::fmt::Display for HeuristicEnum {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            HeuristicEnum::AStar(h) => write!(f, "{}", h),
            HeuristicEnum::WeightedAStar(h) => write!(f, "{}", h),
            HeuristicEnum::Greedy(h) => write!(f, "{}", h),
        }
    }
}

pub trait Frontier {
    fn add(&mut self, state: State);
    fn pop(&mut self) -> Option<State>;
    fn is_empty(&self) -> bool;
    fn size(&self) -> usize;
    fn contains(&self, state: &State) -> bool;
    fn get_name(&self) -> String;
}

pub struct FrontierBFS {
    queue: VecDeque<State>,
    set: HashSet<u64>,
}

impl FrontierBFS {
    pub fn new() -> Self {
        FrontierBFS {
            queue: VecDeque::with_capacity(65536),
            set: HashSet::with_capacity(65536),
        }
    }
}

impl Frontier for FrontierBFS {
    fn add(&mut self, state: State) {
        self.queue.push_back(state.clone());
        self.set.insert(state.calculate_hash());
    }

    fn pop(&mut self) -> Option<State> {
        let state = self.queue.pop_front()?;
        self.set.remove(&state.calculate_hash());
        Some(state)
    }

    fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    fn size(&self) -> usize {
        self.queue.len()
    }

    fn contains(&self, state: &State) -> bool {
        self.set.contains(&state.calculate_hash())
    }

    fn get_name(&self) -> String {
        "breadth-first search".to_string()
    }
}

pub struct FrontierDFS {
    queue: VecDeque<State>,
    set: HashSet<u64>,
}

impl FrontierDFS {
    pub fn new() -> Self {
        FrontierDFS {
            queue: VecDeque::with_capacity(65536),
            set: HashSet::with_capacity(65536),
        }
    }
}

impl Frontier for FrontierDFS {
    fn add(&mut self, state: State) {
        self.queue.push_front(state.clone());
        self.set.insert(state.calculate_hash());
    }

    fn pop(&mut self) -> Option<State> {
        let state = self.queue.pop_front()?;
        self.set.remove(&state.calculate_hash());
        Some(state)
    }

    fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    fn size(&self) -> usize {
        self.queue.len()
    }

    fn contains(&self, state: &State) -> bool {
        self.set.contains(&state.calculate_hash())
    }

    fn get_name(&self) -> String {
        "depth-first search".to_string()
    }
}

// Wrapper for State to implement Ord for BinaryHeap
struct StateWithPriority {
    state: State,
    priority: isize,
}

impl PartialEq for StateWithPriority {
    fn eq(&self, other: &Self) -> bool {
        self.priority == other.priority
    }
}

impl Eq for StateWithPriority {}

impl PartialOrd for StateWithPriority {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for StateWithPriority {
    fn cmp(&self, other: &Self) -> Ordering {
        // Reverse ordering for min-heap
        other.priority.cmp(&self.priority)
    }
}

pub struct FrontierBestFirst {
    heuristic: HeuristicEnum,
    queue: BinaryHeap<StateWithPriority>,
    set: HashSet<u64>,
}

impl FrontierBestFirst {
    pub fn new(heuristic: HeuristicEnum) -> Self {
        FrontierBestFirst {
            heuristic,
            queue: BinaryHeap::new(),
            set: HashSet::with_capacity(65536),
        }
    }
}

impl Frontier for FrontierBestFirst {
    fn add(&mut self, state: State) {
        let priority = self.heuristic.f(&state) as isize;
        self.queue.push(StateWithPriority {
            state: state.clone(),
            priority,
        });
        self.set.insert(state.calculate_hash());
    }

    fn pop(&mut self) -> Option<State> {
        let state_with_priority = self.queue.pop()?;
        self.set.remove(&state_with_priority.state.calculate_hash());
        Some(state_with_priority.state)
    }

    fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    fn size(&self) -> usize {
        self.queue.len()
    }

    fn contains(&self, state: &State) -> bool {
        self.set.contains(&state.calculate_hash())
    }

    fn get_name(&self) -> String {
        format!("best-first search using {}", self.heuristic.to_string())
    }
}

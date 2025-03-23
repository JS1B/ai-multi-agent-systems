use std::collections::HashSet;
use std::time::Instant;

use crate::action::Action;
use crate::frontier::Frontier;
use crate::frontier::FrontierOps;
use crate::memory::Memory;
use crate::state::State;
use crate::BENCHMARK_LOGS;

pub struct SearchState<'a> {
    frontier: Box<dyn Frontier<'a>>,
    expanded: HashSet<State<'a>>,
    iterations: usize,
    start_time: Instant,
}

impl<'a> SearchState<'a> {
    fn new(initial_state: State<'a>, frontier: Box<dyn Frontier<'a>>) -> Self {
        let mut search_state = SearchState {
            frontier,
            expanded: HashSet::new(),
            iterations: 0,
            start_time: Instant::now(),
        };
        search_state.frontier.add(initial_state);
        search_state
    }
    
    fn step(&mut self) -> Option<Vec<Vec<Action>>> {
        // Get next state
        let (state, size) = match self.frontier.get_next_state() {
            Some(result) => result,
            None => {
                self.print_status(0);
                return None;
            }
        };

        // Check if goal reached
        if state.is_goal_state() {
            self.print_status(size);
            return Some(state.extract_plan());
        }

        // Get next states
        let next_states: Vec<_> = state.get_expanded_states()
            .into_iter()
            .filter(|s| !self.expanded.contains(s))
            .collect();
            
        // Update expanded set
        self.expanded.insert(state);
        
        // Add new states to frontier
        for next_state in next_states {
            if !self.frontier.contains(&next_state) {
                self.frontier.add(next_state);
            }
        }

        // Print status if needed
        self.iterations += 1;
        if unsafe { !BENCHMARK_LOGS } && self.iterations % 10000 == 0 {
            self.print_status(size);
        }
        
        None
    }
    
    fn get_next_state(&mut self) -> Option<(State, usize)> {
        if self.frontier.is_empty() {
            None
        } else {
            let size = self.frontier.size();
            self.frontier.pop().map(|state| (state, size))
        }
    }
    
    fn print_status(&self, frontier_size: usize) {
        let elapsed_time = self.start_time.elapsed().as_secs_f64();
        eprintln!(
            "#Expanded: {}, #Frontier: {}, #Generated: {}, Time: {:.3} \n{}\n",
            self.expanded.len(),
            frontier_size,
            self.expanded.len() + frontier_size,
            elapsed_time,
            Memory::new().string_rep()
        );
    }
}

pub struct GraphSearch;

impl GraphSearch {
    pub fn search<'a, F>(
        initial_state: State<'a>,
        mut frontier: F,
    ) -> Option<Vec<Vec<Action>>>
    where
        F: Frontier<'a>,
    {
        let mut iterations = 0;
        let start_time = Instant::now();
        let mut expanded: HashSet<u64> = HashSet::new();

        frontier.add(initial_state);

        while !frontier.is_empty() {
            let size = frontier.size();
            
            // Get next state and process it
            if let Some(state) = frontier.pop() {
                if let Some(plan) = frontier.process_batch(state, &mut expanded) {
                    Self::print_status(&expanded, size, start_time);
                    return Some(plan);
                }
            } else {
                Self::print_status(&expanded, size, start_time);
                return None;
            }

            // Print status if needed
            iterations += 1;
            if unsafe { !BENCHMARK_LOGS } && iterations % 10000 == 0 {
                Self::print_status(&expanded, size, start_time);
            }
        }

        Self::print_status(&expanded, frontier.size(), start_time);
        None
    }

    fn print_status(
        expanded: &HashSet<u64>,
        frontier_size: usize,
        start_time: Instant,
    ) {
        let elapsed_time = start_time.elapsed().as_secs_f64();
        eprintln!(
            "#Expanded: {}, #Frontier: {}, #Generated: {}, Time: {:.3} \n{}\n",
            expanded.len(),
            frontier_size,
            expanded.len() + frontier_size,
            elapsed_time,
            Memory::new().string_rep()
        );
    }
}


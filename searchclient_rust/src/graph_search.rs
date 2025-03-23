use std::collections::HashSet;
use std::time::Instant;

use crate::action::Action;
use crate::frontier::Frontier;
use crate::memory::Memory;
use crate::state::State;
use crate::BENCHMARK_LOGS;

pub struct GraphSearch;

impl GraphSearch {
    pub fn search(
        initial_state: State,
        mut frontier: Box<dyn Frontier>,
    ) -> Option<Vec<Vec<Action>>> {
        let mut iterations = 0;
        let start_time = Instant::now();

        frontier.add(initial_state);
        let mut expanded: HashSet<State> = HashSet::new();

        loop {
            if frontier.is_empty() {
                Self::print_search_status(&expanded, &frontier, start_time);
                return None;
            }

            let s = match frontier.pop() {
                Some(state) => state,
                None => return None,
            };

            if s.is_goal_state() {
                Self::print_search_status(&expanded, &frontier, start_time);
                return Some(s.extract_plan());
            }

            expanded.insert(s);

            for t in s.get_expanded_states() {
                if !frontier.contains(&t) && !expanded.contains(&t) {
                    frontier.add(t);
                }
            }

            // Print a status message every n iteration
            iterations += 1;
            if unsafe { !BENCHMARK_LOGS } && iterations % 10000 == 0 {
                Self::print_search_status(&expanded, &frontier, start_time);
            }
        }
    }

    fn print_search_status(
        expanded: &HashSet<State>,
        frontier: &Box<dyn Frontier>,
        start_time: Instant,
    ) {
        let elapsed_time = start_time.elapsed().as_secs_f64();
        eprintln!(
            "#Expanded: {}, #Frontier: {}, #Generated: {}, Time: {:.3} \n{}\n",
            expanded.len(),
            frontier.size(),
            expanded.len() + frontier.size(),
            elapsed_time,
            Memory::new().string_rep()
        );
    }
}

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
        let output_fixed_solution = false;

        if output_fixed_solution {
            // Part 1:
            // The agents will perform the sequence of actions returned by this method.
            // Try to solve a few levels by hand, enter the found solutions below, and run them:

            Some(vec![
                vec![Action::MoveS],
                vec![Action::MoveS],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveS],
                vec![Action::MoveS],
            ])

            /*
            Some(vec![
                vec![Action::MoveS],
                vec![Action::MoveE],
                vec![Action::MoveE],
                vec![Action::MoveS],
            ])
            */
        } else {
            // Part 2:
            // Now try to implement the Graph-Search algorithm from R&N figure 3.7
            // In the case of "failure to find a solution" you should return None.
            // Some useful methods on the state class which you will need to use are:
            // state.is_goal_state() - Returns true if the state is a goal state.
            // state.extract_plan() - Returns the Array of actions used to reach this state.
            // state.get_expanded_states() - Returns an ArrayList<State> containing the states reachable from the current state.
            // You should also take a look at Frontier.java to see which methods the Frontier interface exposes
            //
            // printSearchStatus(expanded, frontier): As you can see below, the code will print out status
            // (#expanded states, size of the frontier, #generated states, total time used) for every 10000th node generated.
            // You should also make sure to print out these stats when a solution has been found, so you can keep
            // track of the exact total number of states generated!!

            let mut iterations = 0;
            let start_time = Instant::now();

            frontier.add(initial_state);
            let mut expanded = HashSet::new();

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

                expanded.insert(s.calculate_hash());

                for t in s.get_expanded_states() {
                    if !frontier.contains(&t) && !expanded.contains(&t.calculate_hash()) {
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
    }

    fn print_search_status(
        expanded: &HashSet<u64>,
        frontier: &Box<dyn Frontier>,
        start_time: Instant,
    ) {
        let elapsed_time = start_time.elapsed().as_secs_f64();
        eprintln!(
            "#Expanded: {:8}, #Frontier: {:8}, #Generated: {:8}, Time: {:.3} s\n{}\n",
            expanded.len(),
            frontier.size(),
            expanded.len() + frontier.size(),
            elapsed_time,
            Memory::string_rep()
        );
    }
}

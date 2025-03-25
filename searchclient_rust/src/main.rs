mod action;
mod color;
mod frontier;
mod graph_search;
mod heuristic;
mod memory;
mod state;

use std::env;
use std::io::{self, BufRead};
use std::process;
use std::time::Instant;

use action::Action;
use frontier::{Frontier, FrontierBFS, FrontierBestFirst, FrontierDFS, HeuristicEnum};
use graph_search::GraphSearch;
use heuristic::{
    HBoxGoalCount, HGoalCount, HSumDistances, HSumDistancesBox, HSumDistancesBox2, HZero,
    HeuristicAStar, HeuristicGreedy, HeuristicWeightedAStar,
};
use memory::Memory;
use state::State;

pub static mut BENCHMARK_LOGS: bool = false;

fn parse_level(reader: &mut impl BufRead) -> io::Result<State> {
    // We can assume that the level file is conforming to specification, since the server verifies this.
    // Read domain
    let mut line = String::new();
    reader.read_line(&mut line)?; // #domain
    line.clear();
    reader.read_line(&mut line)?; // hospital

    // Read Level name
    line.clear();
    reader.read_line(&mut line)?; // #levelname
    line.clear();
    reader.read_line(&mut line)?; // <n>

    // Read colors
    line.clear();
    reader.read_line(&mut line)?; // #colors
    let mut agent_colors = vec![color::Color::Blue; 10];
    let mut box_colors = vec![color::Color::Blue; 26];

    line.clear();
    reader.read_line(&mut line)?;
    while !line.starts_with('#') {
        let parts: Vec<&str> = line.split(':').collect();
        let color = color::Color::from_string(parts[0].trim());
        let entities: Vec<&str> = parts[1].split(',').collect();

        for entity in entities {
            let c = entity.trim().chars().next().unwrap();
            if c >= '0' && c <= '9' {
                agent_colors[c as usize - '0' as usize] = color;
            } else if c >= 'A' && c <= 'Z' {
                box_colors[c as usize - 'A' as usize] = color;
            }
        }

        line.clear();
        reader.read_line(&mut line)?;
    }

    // Read initial state
    // line is currently "#initial"
    let mut num_rows = 0;
    let mut num_cols = 0;
    let mut level_lines = Vec::with_capacity(64);

    line.clear();
    reader.read_line(&mut line)?;
    while !line.starts_with('#') {
        level_lines.push(line.clone());
        num_cols = num_cols.max(line.trim_end().len());
        num_rows += 1;

        line.clear();
        reader.read_line(&mut line)?;
    }

    let mut num_agents = 0;
    let mut agent_rows = vec![0; 10];
    let mut agent_cols = vec![0; 10];
    let mut walls = vec![vec![false; num_cols]; num_rows];
    let mut boxes = vec![vec!['\0'; num_cols]; num_rows];

    for row in 0..num_rows {
        let line = &level_lines[row];
        for (col, c) in line.trim_end().chars().enumerate() {
            if c >= '0' && c <= '9' {
                agent_rows[c as usize - '0' as usize] = row;
                agent_cols[c as usize - '0' as usize] = col;
                num_agents += 1;
            } else if c >= 'A' && c <= 'Z' {
                boxes[row][col] = c;
            } else if c == '+' {
                walls[row][col] = true;
            }
        }
    }

    agent_rows.truncate(num_agents);
    agent_cols.truncate(num_agents);

    // Read goal state
    // line is currently "#goal"
    let mut goals = vec![vec!['\0'; num_cols]; num_rows];

    line.clear();
    reader.read_line(&mut line)?;
    let mut row = 0;
    while !line.starts_with('#') {
        for (col, c) in line.trim_end().chars().enumerate() {
            if (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') {
                goals[row][col] = c;
            }
        }

        row += 1;
        line.clear();
        reader.read_line(&mut line)?;
    }

    // End
    // line is currently "#end"

    Ok(State::new(
        agent_rows,
        agent_cols,
        agent_colors,
        walls,
        boxes,
        box_colors,
        goals,
    ))
}

fn search(initial_state: State, frontier: Box<dyn Frontier>) -> Option<Vec<Vec<Action>>> {
    eprintln!("Starting {}.", frontier.get_name());
    GraphSearch::search(initial_state, frontier)
}

fn main() -> io::Result<()> {
    // Use stderr to print to the console.
    eprintln!("Rust SearchClient initializing. I am sending this using the error output stream.");

    // Send client name to server.
    println!("SearchClient");

    // We can also print comments to stdout by prefixing with a #.
    println!("#This is a comment.");

    // Parse the level.
    let stdin = io::stdin();
    let mut reader = stdin.lock();
    let initial_state = parse_level(&mut reader)?;

    // Select search strategy.
    let args: Vec<String> = env::args().collect();
    let mut frontier: Box<dyn Frontier> = Box::new(FrontierBFS::new()); // Default

    for i in 1..args.len() {
        match args[i].to_lowercase().as_str() {
            "-benchmark" => unsafe {
                BENCHMARK_LOGS = true;
            },
            "-bfs" => {
                frontier = Box::new(FrontierBFS::new());
            }
            "-dfs" => {
                frontier = Box::new(FrontierDFS::new());
            }
            "-astar" => {
                frontier = Box::new(FrontierBestFirst::new(HeuristicEnum::AStar(
                    HeuristicAStar::new(&initial_state),
                )));
            }
            "-wastar" => {
                let mut w = 5;
                if i + 1 < args.len() {
                    if let Ok(weight) = args[i + 1].parse::<usize>() {
                        w = weight;
                    } else {
                        eprintln!(
                            "Couldn't parse weight argument to -wastar as integer, using default."
                        );
                    }
                }
                frontier = Box::new(FrontierBestFirst::new(HeuristicEnum::WeightedAStar(
                    HeuristicWeightedAStar::new(&initial_state, w),
                )));
            }
            "-greedy" => {
                frontier = Box::new(FrontierBestFirst::new(HeuristicEnum::Greedy(
                    HeuristicGreedy::new(&initial_state),
                )));
            }
            "-heur" => {
                // Specify heuristic function h to be used
                if i + 1 < args.len() {
                    match args[i + 1].to_lowercase().as_str() {
                        "zero" => {
                            heuristic::set_heuristic(Box::new(HZero::new()));
                        }
                        "goalcount" => {
                            heuristic::set_heuristic(Box::new(HGoalCount::new()));
                        }
                        "boxgoalcount" => {
                            heuristic::set_heuristic(Box::new(HBoxGoalCount::new()));
                        }
                        "custom" => {
                            heuristic::set_heuristic(Box::new(HSumDistances::new()));
                        }
                        "boxcustom" => {
                            heuristic::set_heuristic(Box::new(HSumDistancesBox::new()));
                        }
                        "boxcustom2" => {
                            heuristic::set_heuristic(Box::new(HSumDistancesBox2::new()));
                        }
                        _ => {
                            heuristic::set_heuristic(Box::new(HZero::new()));
                            eprintln!("Defaulting to HZero heuristic.");
                        }
                    }
                }
            }
            _ => {
                frontier = Box::new(FrontierBFS::new());
                eprintln!("Defaulting to BFS search. Use arguments -bfs, -dfs, -astar, -wastar, or -greedy to set the search strategy.");
            }
        }
    }

    if args.len() <= 1 {
        frontier = Box::new(FrontierBFS::new());
        eprintln!("Defaulting to BFS search. Use arguments -bfs, -dfs, -astar, -wastar, or -greedy to set the search strategy.");
    }

    // Search for a plan.
    let plan = match search(initial_state, frontier) {
        Some(plan) => plan,
        None => {
            eprintln!("Unable to solve level.");
            process::exit(0);
        }
    };

    // Print plan to server.
    eprintln!("Found solution of length {}.", plan.len());

    for joint_action in plan {
        let mut joint_action_string = String::new();
        for action in &joint_action {
            joint_action_string.push_str(&action.to_string());
            joint_action_string.push('|');
        }
        joint_action_string.pop(); // Remove the last '|'
        println!("{}", joint_action_string);
    }

    Ok(())
}

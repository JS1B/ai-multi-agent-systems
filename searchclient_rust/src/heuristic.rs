use std::fmt;
use std::sync::RwLock;

use crate::state::State;

lazy_static::lazy_static! {
    static ref HEURISTIC: RwLock<Option<Box<dyn CustomH + Send + Sync>>> = RwLock::new(None);
}

pub trait Heuristic {
    fn h(&self, s: &State) -> isize;
    fn f(&self, s: &State) -> isize;
}

// Move the set_heuristic function outside the trait to make it object-safe
pub fn set_heuristic(heur: Box<dyn CustomH + Send + Sync>) {
    let mut heuristic = HEURISTIC.write().unwrap();
    *heuristic = Some(heur);
}

pub trait CustomH: fmt::Display + Send + Sync {
    fn init(&mut self, initial_state: &State);
    fn h(&self, s: &State) -> isize;
}

pub struct HeuristicAStar<'a> {
    _phantom: std::marker::PhantomData<&'a ()>,
}

impl<'a> HeuristicAStar<'a> {
    pub fn new() -> Self {
        HeuristicAStar {
            _phantom: std::marker::PhantomData,
        }
    }
}

impl<'a> Heuristic for HeuristicAStar<'a> {
    fn h(&self, s: &State) -> isize {
        let mut sum = 0;
        let level = s.level;
        let num_rows = level.goals.len();
        let num_cols = level.goals[0].len();

        for row in 1..(num_rows - 1) {
            for col in 1..(num_cols - 1) {
                let goal = level.goals[row][col];
                if goal >= 'A' && goal <= 'Z' {
                    let box_char = s.boxes[row][col];
                    if box_char != goal {
                        sum += 1;
                    }
                }
            }
        }
        sum
    }

    fn f(&self, s: &State) -> isize {
        s.g() as isize + self.h(s)
    }
}

impl<'a> std::fmt::Display for HeuristicAStar<'a> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "A* evaluation")
    }
}

pub struct HeuristicWeightedAStar<'a> {
    weight: f64,
    _phantom: std::marker::PhantomData<&'a ()>,
}

impl<'a> HeuristicWeightedAStar<'a> {
    pub fn new(weight: f64) -> Self {
        HeuristicWeightedAStar {
            weight,
            _phantom: std::marker::PhantomData,
        }
    }
}

impl<'a> Heuristic for HeuristicWeightedAStar<'a> {
    fn h(&self, s: &State) -> isize {
        let mut sum = 0;
        let level = s.level;
        let num_rows = level.goals.len();
        let num_cols = level.goals[0].len();

        for row in 1..(num_rows - 1) {
            for col in 1..(num_cols - 1) {
                let goal = level.goals[row][col];
                if goal >= 'A' && goal <= 'Z' {
                    let box_char = s.boxes[row][col];
                    if box_char != goal {
                        sum += 1;
                    }
                }
            }
        }
        sum
    }

    fn f(&self, s: &State) -> isize {
        s.g() as isize + (self.weight * self.h(s) as f64) as isize
    }
}

impl<'a> std::fmt::Display for HeuristicWeightedAStar<'a> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "WA*({}) evaluation", self.weight)
    }
}

pub struct HeuristicGreedy<'a> {
    _phantom: std::marker::PhantomData<&'a ()>,
}

impl<'a> HeuristicGreedy<'a> {
    pub fn new() -> Self {
        HeuristicGreedy {
            _phantom: std::marker::PhantomData,
        }
    }
}

impl<'a> Heuristic for HeuristicGreedy<'a> {
    fn h(&self, s: &State) -> isize {
        let mut sum = 0;
        let level = s.level;
        let num_rows = level.goals.len();
        let num_cols = level.goals[0].len();

        for row in 1..(num_rows - 1) {
            for col in 1..(num_cols - 1) {
                let goal = level.goals[row][col];
                if goal >= 'A' && goal <= 'Z' {
                    let box_char = s.boxes[row][col];
                    if box_char != goal {
                        sum += 1;
                    }
                }
            }
        }
        sum
    }

    fn f(&self, s: &State) -> isize {
        self.h(s)
    }
}

impl<'a> std::fmt::Display for HeuristicGreedy<'a> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "greedy evaluation")
    }
}

// Heuristic implementations
pub struct HZero;

impl HZero {
    pub fn new() -> Self {
        HZero
    }
}

impl CustomH for HZero {
    fn init(&mut self, _initial_state: &State) {
        // No initialization needed
    }

    fn h(&self, _s: &State) -> isize {
        0
    }
}

impl fmt::Display for HZero {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Zero")
    }
}

pub struct HGoalCount;

impl HGoalCount {
    pub fn new() -> Self {
        HGoalCount
    }
}

impl CustomH for HGoalCount {
    fn init(&mut self, _initial_state: &State) {
        // No initialization needed
    }

    fn h(&self, s: &State) -> isize {
        let mut count = 0;

        for row in 0..s.boxes.len() {
            for col in 0..s.boxes[row].len() {
                let goal = s.level.goals[row][col];
                if goal >= 'A' && goal <= 'Z' && s.boxes[row][col] != goal {
                    count += 1;
                }
                if goal >= '0' && goal <= '9' {
                    let agent_idx = (goal as u8 - b'0') as usize;
                    if agent_idx >= s.agent_rows.len()
                        || s.agent_rows[agent_idx] != row as i32
                        || s.agent_cols[agent_idx] != col as i32
                    {
                        count += 1;
                    }
                }
            }
        }

        count
    }
}

impl fmt::Display for HGoalCount {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "GoalCount")
    }
}

pub struct HBoxGoalCount;

impl HBoxGoalCount {
    pub fn new() -> Self {
        HBoxGoalCount
    }
}

impl CustomH for HBoxGoalCount {
    fn init(&mut self, _initial_state: &State) {
        // No initialization needed
    }

    fn h(&self, s: &State) -> isize {
        let mut count = 0;

        for row in 0..s.boxes.len() {
            for col in 0..s.boxes[row].len() {
                let goal = s.level.goals[row][col];
                if goal >= 'A' && goal <= 'Z' && s.boxes[row][col] != goal {
                    count += 1;
                }
            }
        }

        count
    }
}

impl fmt::Display for HBoxGoalCount {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "BoxGoalCount")
    }
}

pub struct HSumDistances;

impl HSumDistances {
    pub fn new() -> Self {
        HSumDistances
    }
}

impl CustomH for HSumDistances {
    fn init(&mut self, _initial_state: &State) {
        // No initialization needed
    }

    fn h(&self, s: &State) -> isize {
        let mut sum = 0;

        // Sum of Manhattan distances from each agent to its goal
        for agent_idx in 0..s.agent_rows.len() {
            let agent_row = s.agent_rows[agent_idx];
            let agent_col = s.agent_cols[agent_idx];
            let agent_char = (agent_idx as u8 + b'0') as char;

            // Find the goal for this agent
            let mut goal_found = false;
            for row in 0..s.level.goals.len() {
                for col in 0..s.level.goals[row].len() {
                    if s.level.goals[row][col] == agent_char {
                        sum += (agent_row as isize - row as isize).abs()
                            + (agent_col as isize - col as isize).abs();
                        goal_found = true;
                        break;
                    }
                }
                if goal_found {
                    break;
                }
            }
        }

        sum
    }
}

impl fmt::Display for HSumDistances {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "SumDistances")
    }
}

pub struct HSumDistancesBox;

impl HSumDistancesBox {
    pub fn new() -> Self {
        HSumDistancesBox
    }
}

impl CustomH for HSumDistancesBox {
    fn init(&mut self, _initial_state: &State) {
        // No initialization needed
    }

    fn h(&self, s: &State) -> isize {
        let mut sum = 0;

        // Sum of Manhattan distances from each box to its goal
        for row in 0..s.boxes.len() {
            for col in 0..s.boxes[row].len() {
                let box_char = s.boxes[row][col];
                if box_char >= 'A' && box_char <= 'Z' {
                    // Find the goal for this box
                    let mut goal_found = false;
                    for goal_row in 0..s.level.goals.len() {
                        for goal_col in 0..s.level.goals[goal_row].len() {
                            if s.level.goals[goal_row][goal_col] == box_char {
                                sum += (row as isize - goal_row as isize).abs()
                                    + (col as isize - goal_col as isize).abs();
                                goal_found = true;
                                break;
                            }
                        }
                        if goal_found {
                            break;
                        }
                    }
                }
            }
        }

        sum
    }
}

impl fmt::Display for HSumDistancesBox {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "SumDistancesBox")
    }
}

pub struct HSumDistancesBox2;

impl HSumDistancesBox2 {
    pub fn new() -> Self {
        HSumDistancesBox2
    }
}

impl CustomH for HSumDistancesBox2 {
    fn init(&mut self, _initial_state: &State) {
        // No initialization needed
    }

    fn h(&self, s: &State) -> isize {
        let mut sum = 0;

        // Sum of Manhattan distances from each box to its goal
        for row in 0..s.boxes.len() {
            for col in 0..s.boxes[row].len() {
                let box_char = s.boxes[row][col];
                if box_char >= 'A' && box_char <= 'Z' {
                    // Find the goal for this box
                    let mut goal_found = false;
                    for goal_row in 0..s.level.goals.len() {
                        for goal_col in 0..s.level.goals[goal_row].len() {
                            if s.level.goals[goal_row][goal_col] == box_char {
                                sum += (row as isize - goal_row as isize).abs()
                                    + (col as isize - goal_col as isize).abs();
                                goal_found = true;
                                break;
                            }
                        }
                        if goal_found {
                            break;
                        }
                    }
                }
            }
        }

        // Add distances from agents to nearest boxes of their color
        for agent_idx in 0..s.agent_rows.len() {
            let agent_row = s.agent_rows[agent_idx];
            let agent_col = s.agent_cols[agent_idx];
            let agent_color = s.level.agent_colors[agent_idx];

            let mut min_distance = std::isize::MAX;

            // Find the nearest box of the same color
            for row in 0..s.boxes.len() {
                for col in 0..s.boxes[row].len() {
                    let box_char = s.boxes[row][col];
                    if box_char >= 'A' && box_char <= 'Z' {
                        let box_color = s.level.box_colors[(box_char as u8 - b'A') as usize];
                        if box_color == agent_color {
                            let distance = (agent_row as isize - row as isize).abs()
                                + (agent_col as isize - col as isize).abs();
                            min_distance = min_distance.min(distance);
                        }
                    }
                }
            }

            if min_distance != std::isize::MAX {
                sum += min_distance;
            }
        }

        sum
    }
}

impl fmt::Display for HSumDistancesBox2 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "SumDistancesBox2")
    }
}

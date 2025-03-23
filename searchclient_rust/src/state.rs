// state.rs

use crate::action::{Action, Direction};
use crate::color::Color;
use rand::rngs::StdRng;
use rand::seq::SliceRandom;
use rand::SeedableRng;
use std::fmt;
use std::hash::{Hash, Hasher};
use std::collections::hash_map::DefaultHasher;

/// Level holds immutable board data.
/// This data is read from input once and then passed as a reference to every State.
pub struct Level {
    pub walls: Vec<Vec<bool>>,    // row-major grid of walls (true means wall)
    pub goals: Vec<Vec<char>>,    // board goals (space if none)
    pub agent_colors: Vec<Color>, // indexed by agent number
    pub box_colors: Vec<Color>,   // indexed alphabetically (A=0, B=1, etc.)
}

/// A State represents a configuration of the puzzle.
/// It holds agent positions, the dynamic board (boxes),
/// a pointer to its parent state (for extracting a plan),
/// the joint action that led here, and the cost (g).
/// Each State also has a reference to the immutable Level.
// #[derive(Clone, PartialEq, Eq, Hash, Debug)]
#[derive(Clone)]
pub struct State<'a> {
    pub level: &'a Level,
    pub agent_rows: Vec<i32>,
    pub agent_cols: Vec<i32>,
    pub boxes: Vec<Vec<char>>,
    pub parent: Option<Box<State<'a>>>,
    pub joint_action: Option<Vec<Action>>,
    pub g: i32,
    hash: Option<u64>, // cached hash (if needed)
}

impl<'a> State<'a> {
    /// Constructs an initial state.
    /// Assumes level is already initialized and provided as a reference.
    pub fn new(
        level: &'a Level,
        agent_rows: Vec<i32>,
        agent_cols: Vec<i32>,
        boxes: Vec<Vec<char>>,
    ) -> Self {
        State {
            level,
            agent_rows,
            agent_cols,
            boxes,
            parent: None,
            joint_action: None,
            g: 0,
            hash: None,
        }
    }

    /// Constructs the state resulting from applying a joint action in the parent state.
    /// Precondition: joint_action is applicable and non-conflicting.
    pub fn apply_joint_action(parent: &State<'a>, joint_action: &[Action]) -> Self {
        let agent_rows = parent.agent_rows.clone();
        let agent_cols = parent.agent_cols.clone();
        let boxes = parent
            .boxes
            .iter()
            .map(|row| row.clone())
            .collect::<Vec<_>>();

        let mut new_state = State {
            level: parent.level,
            agent_rows,
            agent_cols,
            boxes,
            parent: Some(Box::new(parent.clone())),
            joint_action: Some(joint_action.to_vec()),
            g: parent.g + 1,
            hash: None,
        };

        let num_agents = new_state.agent_rows.len();
        for agent in 0..num_agents {
            let action = joint_action[agent];
            match action {
                Action::NoOp => {
                    // Do nothing.
                }
                Action::Move(dir) => {
                    let (dr, dc) = dir.delta();
                    new_state.agent_rows[agent] += dr;
                    new_state.agent_cols[agent] += dc;
                }
                Action::Push {
                    agent: agent_dir,
                    box_move,
                } => {
                    let (dr, dc) = agent_dir.delta();
                    let box_row = new_state.agent_rows[agent] + dr;
                    let box_col = new_state.agent_cols[agent] + dc;
                    // Agent moves.
                    new_state.agent_rows[agent] += dr;
                    new_state.agent_cols[agent] += dc;
                    let (bdr, bdc) = box_move.delta();
                    let dest_row = box_row + bdr;
                    let dest_col = box_col + bdc;
                    new_state.boxes[dest_row as usize][dest_col as usize] =
                        new_state.boxes[box_row as usize][box_col as usize];
                    new_state.boxes[box_row as usize][box_col as usize] = '\0';
                }
                Action::Pull {
                    agent: agent_dir,
                    box_move,
                } => {
                    let (dr, dc) = agent_dir.delta();
                    // The box is behind the agent.
                    let box_row = new_state.agent_rows[agent] - box_move.delta().0;
                    let box_col = new_state.agent_cols[agent] - box_move.delta().1;
                    // Agent moves.
                    new_state.agent_rows[agent] += dr;
                    new_state.agent_cols[agent] += dc;
                    let (bdr, bdc) = box_move.delta();
                    let dest_row = box_row + bdr;
                    let dest_col = box_col + bdc;
                    new_state.boxes[dest_row as usize][dest_col as usize] =
                        new_state.boxes[box_row as usize][box_col as usize];
                    new_state.boxes[box_row as usize][box_col as usize] = '\0';
                }
            }
        }
        new_state
    }

    /// Returns the cost (g) of the state.
    pub fn g(&self) -> i32 {
        self.g
    }

    /// Checks if the state is a goal state.
    /// For each cell (ignoring borders) if there's a goal for a box (an uppercase letter)
    /// or for an agent (a digit), the corresponding box or agent must be present.
    pub fn is_goal_state(&self) -> bool {
        let level = self.level;
        let num_rows = level.goals.len();
        let num_cols = level.goals[0].len();
        for row in 1..(num_rows - 1) {
            for col in 1..(num_cols - 1) {
                let goal = level.goals[row][col];
                if goal >= 'A' && goal <= 'Z' {
                    if self.boxes[row][col] != goal {
                        return false;
                    }
                } else if goal >= '0' && goal <= '9' {
                    let agent_index = (goal as u8 - b'0') as usize;
                    if self.agent_rows[agent_index] != row as i32
                        || self.agent_cols[agent_index] != col as i32
                    {
                        return false;
                    }
                }
            }
        }
        true
    }

    /// Generates all expanded states from this state by applying every non-conflicting joint action.
    pub fn get_expanded_states(&self) -> Vec<State<'a>> {
        let num_agents = self.agent_rows.len();
        let all_actions = all_actions();
        let mut applicable_actions: Vec<Vec<Action>> = Vec::with_capacity(num_agents);
        for agent in 0..num_agents {
            let mut agent_actions = Vec::new();
            for action in &all_actions {
                if self.is_applicable(agent, *action) {
                    agent_actions.push(*action);
                }
            }
            applicable_actions.push(agent_actions);
        }

        let mut expanded_states = Vec::new();
        let mut joint_action = vec![Action::NoOp; num_agents];
        let mut indices = vec![0; num_agents];

        loop {
            for agent in 0..num_agents {
                joint_action[agent] = applicable_actions[agent][indices[agent]];
            }
            if !self.is_conflicting(&joint_action) {
                let child = State::apply_joint_action(self, &joint_action);
                expanded_states.push(child);
            }
            let mut done = false;
            for i in 0..num_agents {
                if indices[i] < applicable_actions[i].len() - 1 {
                    indices[i] += 1;
                    break;
                } else {
                    indices[i] = 0;
                    if i == num_agents - 1 {
                        done = true;
                    }
                }
            }
            if done {
                break;
            }
        }

        // Shuffle expanded states using a fixed-seed RNG (to mimic Java's Random(1))
        let mut rng = StdRng::seed_from_u64(1);
        expanded_states.shuffle(&mut rng);
        expanded_states
    }

    /// Determines whether the given action is applicable for the specified agent.
    pub fn is_applicable(&self, agent: usize, action: Action) -> bool {
        let level = self.level;
        let agent_row = self.agent_rows[agent];
        let agent_col = self.agent_cols[agent];
        let agent_color = level.agent_colors[agent];
        match action {
            Action::NoOp => true,
            Action::Move(dir) => {
                let (dr, dc) = dir.delta();
                let dest_row = agent_row + dr;
                let dest_col = agent_col + dc;
                self.cell_is_free(dest_row, dest_col)
            }
            Action::Push {
                agent: dir,
                box_move,
            } => {
                let (dr, dc) = dir.delta();
                let box_row = agent_row + dr;
                let box_col = agent_col + dc;
                let box_char = self.boxes[box_row as usize][box_col as usize];
                let (bdr, bdc) = box_move.delta();
                let dest_row = box_row + bdr;
                let dest_col = box_col + bdc;
                box_char != '\0'
                    && agent_color == level.box_colors[(box_char as u8 - b'A') as usize]
                    && self.cell_is_free(dest_row, dest_col)
            }
            Action::Pull {
                agent: dir,
                box_move,
            } => {
                let (dr, dc) = dir.delta();
                let dest_row = agent_row + dr;
                let dest_col = agent_col + dc;
                let (bdr, bdc) = box_move.delta();
                let box_row = agent_row - bdr;
                let box_col = agent_col - bdc;
                let box_char = self.boxes[box_row as usize][box_col as usize];
                box_char != '\0'
                    && agent_color == level.box_colors[(box_char as u8 - b'A') as usize]
                    && self.cell_is_free(dest_row, dest_col)
            }
        }
    }

    /// Checks if a joint action (an action per agent) is conflicting.
    /// For example, if two agents try to move into the same cell.
    pub fn is_conflicting(&self, joint_action: &[Action]) -> bool {
        let num_agents = self.agent_rows.len();
        let mut destination_rows = vec![0; num_agents];
        let mut destination_cols = vec![0; num_agents];
        let mut box_rows = vec![0; num_agents];
        let mut box_cols = vec![0; num_agents];

        for agent in 0..num_agents {
            let action = joint_action[agent];
            let agent_row = self.agent_rows[agent];
            let agent_col = self.agent_cols[agent];
            match action {
                Action::NoOp => {}
                Action::Move(dir) => {
                    let (dr, dc) = dir.delta();
                    destination_rows[agent] = agent_row + dr;
                    destination_cols[agent] = agent_col + dc;
                    box_rows[agent] = agent_row;
                    box_cols[agent] = agent_col;
                }
                Action::Push {
                    agent: dir,
                    box_move,
                } => {
                    let (dr, dc) = dir.delta();
                    let b_row = agent_row + dr;
                    let b_col = agent_col + dc;
                    let (bdr, bdc) = box_move.delta();
                    destination_rows[agent] = b_row + bdr;
                    destination_cols[agent] = b_col + bdc;
                    box_rows[agent] = b_row;
                    box_cols[agent] = b_col;
                }
                Action::Pull {
                    agent: dir,
                    box_move,
                } => {
                    let (dr, dc) = dir.delta();
                    destination_rows[agent] = agent_row + dr;
                    destination_cols[agent] = agent_col + dc;
                    let (bdr, bdc) = box_move.delta();
                    let b_row = agent_row - bdr;
                    let b_col = agent_col - bdc;
                    box_rows[agent] = b_row;
                    box_cols[agent] = b_col;
                }
            }
        }

        for a1 in 0..num_agents {
            if joint_action[a1] == Action::NoOp {
                continue;
            }
            for a2 in (a1 + 1)..num_agents {
                if joint_action[a2] == Action::NoOp {
                    continue;
                }
                if destination_rows[a1] == destination_rows[a2]
                    && destination_cols[a1] == destination_cols[a2]
                {
                    return true;
                }
            }
        }
        false
    }

    /// Returns true if the cell at (row, col) is free:
    /// not a wall, no box is present, and no agent occupies the cell.
    pub fn cell_is_free(&self, row: i32, col: i32) -> bool {
        let level = self.level;
        let r = row as usize;
        let c = col as usize;
        !level.walls[r][c] && self.boxes[r][c] == '\0' && self.agent_at(row, col) == '\0'
    }

    /// Returns the character representing an agent at the given cell,
    /// or '\0' if none is present.
    pub fn agent_at(&self, row: i32, col: i32) -> char {
        for (i, (&r, &c)) in self.agent_rows.iter().zip(&self.agent_cols).enumerate() {
            if r == row && c == col {
                return std::char::from_digit(i as u32, 10).unwrap_or('\0');
            }
        }
        '\0'
    }

    /// Extracts the plan (the sequence of joint actions) from the initial state to this state.
    /// The returned vector is ordered from the first action to the last.
    pub fn extract_plan(&self) -> Vec<Vec<Action>> {
        let mut plan = Vec::with_capacity(self.g as usize);
        let mut state = self;
        while let Some(ref joint) = state.joint_action {
            plan.push(joint.clone());
            state = state.parent.as_ref().unwrap();
        }
        plan.reverse();
        plan
    }

    pub fn calculate_hash(&self) -> u64 {
        let mut hasher = DefaultHasher::new();
        self.agent_rows.hash(&mut hasher);
        self.agent_cols.hash(&mut hasher);
        self.boxes.hash(&mut hasher);
        self.parent.hash(&mut hasher);
        self.g.hash(&mut hasher);
        hasher.finish()
    }
}

impl<'a> fmt::Display for State<'a> {
    /// Pretty-prints the state: printing boxes, walls, agent digits, or spaces.
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let level = self.level;
        let mut s = String::new();
        let rows = level.walls.len();
        let cols = level.walls[0].len();
        for row in 0..rows {
            for col in 0..cols {
                if self.boxes[row][col] != '\0' {
                    s.push(self.boxes[row][col]);
                } else if level.walls[row][col] {
                    s.push('+');
                } else if self.agent_at(row as i32, col as i32) != '\0' {
                    s.push(self.agent_at(row as i32, col as i32));
                } else {
                    s.push(' ');
                }
            }
            s.push('\n');
        }
        write!(f, "{}", s)
    }
}

/// Helper function returning a list of all possible actions.
/// This includes NoOp, moves (four directions), and all Push/Pull combinations.
fn all_actions() -> Vec<Action> {
    use Direction::*;
    let mut actions = Vec::new();
    actions.push(Action::NoOp);
    for &dir in &[N, S, E, W] {
        actions.push(Action::Move(dir));
    }
    for &agent_dir in &[N, S, E, W] {
        for &box_dir in &[N, S, E, W] {
            actions.push(Action::Push {
                agent: agent_dir,
                box_move: box_dir,
            });
        }
    }
    for &agent_dir in &[N, S, E, W] {
        for &box_dir in &[N, S, E, W] {
            actions.push(Action::Pull {
                agent: agent_dir,
                box_move: box_dir,
            });
        }
    }
    actions
}

impl<'a> PartialEq for State<'a> {
    fn eq(&self, other: &Self) -> bool {
        // Equality based on immutable state configuration.
        std::ptr::eq(self.level, other.level)
            && self.agent_rows == other.agent_rows
            && self.agent_cols == other.agent_cols
            && self.boxes == other.boxes
    }
}

impl<'a> Eq for State<'a> {}

impl<'a> Hash for State<'a> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        // Hash the pointer address of the immutable Level.
        (self.level as *const Level).hash(state);
        self.agent_rows.hash(state);
        self.agent_cols.hash(state);
        self.boxes.hash(state);
    }
}

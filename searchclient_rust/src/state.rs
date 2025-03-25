use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};
use std::rc::Rc;

use crate::action::Action;
use crate::color::Color;

pub struct State {
    // Agent positions
    pub agent_rows: Vec<usize>,
    pub agent_cols: Vec<usize>,

    // Static data shared across all states
    pub agent_colors: Rc<Vec<Color>>,
    pub walls: Rc<Vec<Vec<bool>>>,
    pub boxes: Vec<Vec<char>>,
    pub box_colors: Rc<Vec<Color>>,
    pub goals: Rc<Vec<Vec<char>>>,

    // State tracking
    pub parent: Option<Rc<State>>,
    pub joint_action: Option<Vec<Action>>,
    pub g: usize,

    // Caching
    hash: u64,
}

impl State {
    // Constructs an initial state
    pub fn new(
        agent_rows: Vec<usize>,
        agent_cols: Vec<usize>,
        agent_colors: Vec<Color>,
        walls: Vec<Vec<bool>>,
        boxes: Vec<Vec<char>>,
        box_colors: Vec<Color>,
        goals: Vec<Vec<char>>,
    ) -> Self {
        let mut state = State {
            agent_rows,
            agent_cols,
            agent_colors: Rc::new(agent_colors),
            walls: Rc::new(walls),
            boxes,
            box_colors: Rc::new(box_colors),
            goals: Rc::new(goals),
            parent: None,
            joint_action: None,
            g: 0,
            hash: 0,
        };

        // Calculate hash
        state.hash = state.calculate_hash();

        state
    }

    // Constructs a state resulting from applying joint_action in parent
    fn child_state(parent: &Rc<State>, joint_action: Vec<Action>) -> Self {
        // Copy parent
        let mut agent_rows = parent.agent_rows.clone();
        let mut agent_cols = parent.agent_cols.clone();
        let mut boxes = parent.boxes.clone();

        // Apply each action
        let num_agents = agent_rows.len();
        for agent in 0..num_agents {
            let action = joint_action[agent];

            match action.get_type() {
                crate::action::ActionType::NoOp => {
                    // Do nothing
                }
                crate::action::ActionType::Move => {
                    agent_rows[agent] =
                        (agent_rows[agent] as isize + action.agent_row_delta()) as usize;
                    agent_cols[agent] =
                        (agent_cols[agent] as isize + action.agent_col_delta()) as usize;
                }
                crate::action::ActionType::Push => {
                    // Move box
                    let box_row = (agent_rows[agent] as isize + action.agent_row_delta()) as usize;
                    let box_col = (agent_cols[agent] as isize + action.agent_col_delta()) as usize;
                    let box_char = boxes[box_row][box_col];

                    let new_box_row = (box_row as isize + action.box_row_delta()) as usize;
                    let new_box_col = (box_col as isize + action.box_col_delta()) as usize;

                    boxes[box_row][box_col] = '\0';
                    boxes[new_box_row][new_box_col] = box_char;

                    // Move agent
                    agent_rows[agent] = box_row;
                    agent_cols[agent] = box_col;
                }
                crate::action::ActionType::Pull => {
                    // Move agent
                    let new_agent_row =
                        (agent_rows[agent] as isize + action.agent_row_delta()) as usize;
                    let new_agent_col =
                        (agent_cols[agent] as isize + action.agent_col_delta()) as usize;

                    // Move box
                    let box_row = (agent_rows[agent] as isize - action.agent_row_delta()
                        + action.box_row_delta()) as usize;
                    let box_col = (agent_cols[agent] as isize - action.agent_col_delta()
                        + action.box_col_delta()) as usize;
                    let box_char = boxes[box_row][box_col];

                    boxes[box_row][box_col] = '\0';
                    boxes[agent_rows[agent]][agent_cols[agent]] = box_char;

                    agent_rows[agent] = new_agent_row;
                    agent_cols[agent] = new_agent_col;
                }
            }
        }

        let mut state = State {
            agent_rows,
            agent_cols,
            agent_colors: Rc::clone(&parent.agent_colors),
            walls: Rc::clone(&parent.walls),
            boxes,
            box_colors: Rc::clone(&parent.box_colors),
            goals: Rc::clone(&parent.goals),
            parent: Some(Rc::clone(parent)),
            joint_action: Some(joint_action),
            g: parent.g + 1,
            hash: 0,
        };

        // Calculate hash
        state.hash = state.calculate_hash();

        state
    }

    pub fn g(&self) -> usize {
        self.g
    }

    pub fn is_goal_state(&self) -> bool {
        for row in 0..self.boxes.len() {
            for col in 0..self.boxes[row].len() {
                let goal = self.goals[row][col];
                if goal >= 'A' && goal <= 'Z' && self.boxes[row][col] != goal {
                    return false;
                }
                if goal >= '0' && goal <= '9' {
                    let agent_idx = (goal as u8 - b'0') as usize;
                    if agent_idx >= self.agent_rows.len()
                        || self.agent_rows[agent_idx] != row
                        || self.agent_cols[agent_idx] != col
                    {
                        return false;
                    }
                }
            }
        }
        true
    }

    pub fn get_expanded_states(&self) -> Vec<State> {
        let mut expanded_states = Vec::new();

        // Get applicable actions for each agent
        let num_agents = self.agent_rows.len();
        let mut applicable_actions: Vec<Vec<Action>> = Vec::with_capacity(num_agents);

        for agent in 0..num_agents {
            let mut actions = Vec::new();

            // Add NoOp
            actions.push(Action::NoOp);

            // Try Move actions
            if self.is_applicable(agent, Action::MoveN) {
                actions.push(Action::MoveN);
            }
            if self.is_applicable(agent, Action::MoveS) {
                actions.push(Action::MoveS);
            }
            if self.is_applicable(agent, Action::MoveE) {
                actions.push(Action::MoveE);
            }
            if self.is_applicable(agent, Action::MoveW) {
                actions.push(Action::MoveW);
            }

            // Try Push actions
            if self.is_applicable(agent, Action::PushNN) {
                actions.push(Action::PushNN);
            }
            if self.is_applicable(agent, Action::PushNE) {
                actions.push(Action::PushNE);
            }
            if self.is_applicable(agent, Action::PushNW) {
                actions.push(Action::PushNW);
            }
            if self.is_applicable(agent, Action::PushSS) {
                actions.push(Action::PushSS);
            }
            if self.is_applicable(agent, Action::PushSE) {
                actions.push(Action::PushSE);
            }
            if self.is_applicable(agent, Action::PushSW) {
                actions.push(Action::PushSW);
            }
            if self.is_applicable(agent, Action::PushEE) {
                actions.push(Action::PushEE);
            }
            if self.is_applicable(agent, Action::PushEN) {
                actions.push(Action::PushEN);
            }
            if self.is_applicable(agent, Action::PushES) {
                actions.push(Action::PushES);
            }
            if self.is_applicable(agent, Action::PushWW) {
                actions.push(Action::PushWW);
            }
            if self.is_applicable(agent, Action::PushWN) {
                actions.push(Action::PushWN);
            }
            if self.is_applicable(agent, Action::PushWS) {
                actions.push(Action::PushWS);
            }

            // Try Pull actions
            if self.is_applicable(agent, Action::PullNN) {
                actions.push(Action::PullNN);
            }
            if self.is_applicable(agent, Action::PullNE) {
                actions.push(Action::PullNE);
            }
            if self.is_applicable(agent, Action::PullNW) {
                actions.push(Action::PullNW);
            }
            if self.is_applicable(agent, Action::PullSS) {
                actions.push(Action::PullSS);
            }
            if self.is_applicable(agent, Action::PullSE) {
                actions.push(Action::PullSE);
            }
            if self.is_applicable(agent, Action::PullSW) {
                actions.push(Action::PullSW);
            }
            if self.is_applicable(agent, Action::PullEE) {
                actions.push(Action::PullEE);
            }
            if self.is_applicable(agent, Action::PullEN) {
                actions.push(Action::PullEN);
            }
            if self.is_applicable(agent, Action::PullES) {
                actions.push(Action::PullES);
            }
            if self.is_applicable(agent, Action::PullWW) {
                actions.push(Action::PullWW);
            }
            if self.is_applicable(agent, Action::PullWN) {
                actions.push(Action::PullWN);
            }
            if self.is_applicable(agent, Action::PullWS) {
                actions.push(Action::PullWS);
            }

            applicable_actions.push(actions);
        }

        // Generate joint actions
        self.generate_joint_actions(
            &applicable_actions,
            &mut Vec::with_capacity(num_agents),
            0,
            &mut expanded_states,
        );

        expanded_states
    }

    fn generate_joint_actions(
        &self,
        applicable_actions: &[Vec<Action>],
        current_joint_action: &mut Vec<Action>,
        agent_idx: usize,
        expanded_states: &mut Vec<State>,
    ) {
        if agent_idx == applicable_actions.len() {
            // We have a complete joint action, check if it's valid
            if !self.is_conflicting(current_joint_action) {
                let self_rc = Rc::new(self.clone());
                expanded_states.push(State::child_state(&self_rc, current_joint_action.clone()));
            }
            return;
        }

        for &action in &applicable_actions[agent_idx] {
            current_joint_action.push(action);
            self.generate_joint_actions(
                applicable_actions,
                current_joint_action,
                agent_idx + 1,
                expanded_states,
            );
            current_joint_action.pop();
        }
    }

    fn is_applicable(&self, agent: usize, action: Action) -> bool {
        let agent_row = self.agent_rows[agent];
        let agent_col = self.agent_cols[agent];
        let agent_color = self.agent_colors[agent];

        match action.get_type() {
            crate::action::ActionType::NoOp => true,
            crate::action::ActionType::Move => {
                let new_row = (agent_row as isize + action.agent_row_delta()) as usize;
                let new_col = (agent_col as isize + action.agent_col_delta()) as usize;

                self.cell_is_free(new_row, new_col)
            }
            crate::action::ActionType::Push => {
                let box_row = (agent_row as isize + action.agent_row_delta()) as usize;
                let box_col = (agent_col as isize + action.agent_col_delta()) as usize;

                let new_box_row = (box_row as isize + action.box_row_delta()) as usize;
                let new_box_col = (box_col as isize + action.box_col_delta()) as usize;

                // Check if there's a box at the box position
                if self.boxes[box_row][box_col] == '\0' {
                    return false;
                }

                // Check if the box has the same color as the agent
                let box_color =
                    self.box_colors[(self.boxes[box_row][box_col] as u8 - b'A') as usize];
                if box_color != agent_color {
                    return false;
                }

                // Check if the new box position is free
                self.cell_is_free(new_box_row, new_box_col)
            }
            crate::action::ActionType::Pull => {
                let new_agent_row = (agent_row as isize + action.agent_row_delta()) as usize;
                let new_agent_col = (agent_col as isize + action.agent_col_delta()) as usize;

                let box_row = (agent_row as isize - action.agent_row_delta()
                    + action.box_row_delta()) as usize;
                let box_col = (agent_col as isize - action.agent_col_delta()
                    + action.box_col_delta()) as usize;

                // Check if the new agent position is free
                if !self.cell_is_free(new_agent_row, new_agent_col) {
                    return false;
                }

                // Check if there's a box at the box position
                if self.boxes[box_row][box_col] == '\0' {
                    return false;
                }

                // Check if the box has the same color as the agent
                let box_color =
                    self.box_colors[(self.boxes[box_row][box_col] as u8 - b'A') as usize];
                box_color == agent_color
            }
        }
    }

    fn is_conflicting(&self, joint_action: &[Action]) -> bool {
        let num_agents = joint_action.len();

        // Check for conflicts between actions
        for i in 0..num_agents {
            let action_i = joint_action[i];
            let agent_i_row = self.agent_rows[i];
            let agent_i_col = self.agent_cols[i];

            // Destination cell of agent i
            let agent_i_new_row = (agent_i_row as isize + action_i.agent_row_delta()) as usize;
            let agent_i_new_col = (agent_i_col as isize + action_i.agent_col_delta()) as usize;

            // Box movement for agent i
            let mut box_i_row = 0;
            let mut box_i_col = 0;
            let mut box_i_new_row = 0;
            let mut box_i_new_col = 0;

            if action_i.get_type() == crate::action::ActionType::Push {
                box_i_row = agent_i_new_row;
                box_i_col = agent_i_new_col;
                box_i_new_row = (box_i_row as isize + action_i.box_row_delta()) as usize;
                box_i_new_col = (box_i_col as isize + action_i.box_col_delta()) as usize;
            } else if action_i.get_type() == crate::action::ActionType::Pull {
                box_i_row = (agent_i_row as isize - action_i.agent_row_delta()
                    + action_i.box_row_delta()) as usize;
                box_i_col = (agent_i_col as isize - action_i.agent_col_delta()
                    + action_i.box_col_delta()) as usize;
                box_i_new_row = agent_i_row;
                box_i_new_col = agent_i_col;
            }

            // Check for conflicts with other agents
            for j in 0..num_agents {
                if i == j {
                    continue;
                }

                let action_j = joint_action[j];
                let agent_j_row = self.agent_rows[j];
                let agent_j_col = self.agent_cols[j];

                // Destination cell of agent j
                let agent_j_new_row = (agent_j_row as isize + action_j.agent_row_delta()) as usize;
                let agent_j_new_col = (agent_j_col as isize + action_j.agent_col_delta()) as usize;

                // Box movement for agent j
                let mut box_j_row = 0;
                let mut box_j_col = 0;
                let mut box_j_new_row = 0;
                let mut box_j_new_col = 0;

                if action_j.get_type() == crate::action::ActionType::Push {
                    box_j_row = agent_j_new_row;
                    box_j_col = agent_j_new_col;
                    box_j_new_row = (box_j_row as isize + action_j.box_row_delta()) as usize;
                    box_j_new_col = (box_j_col as isize + action_j.box_col_delta()) as usize;
                } else if action_j.get_type() == crate::action::ActionType::Pull {
                    box_j_row = (agent_j_row as isize - action_j.agent_row_delta()
                        + action_j.box_row_delta()) as usize;
                    box_j_col = (agent_j_col as isize - action_j.agent_col_delta()
                        + action_j.box_col_delta()) as usize;
                    box_j_new_row = agent_j_row;
                    box_j_new_col = agent_j_col;
                }

                // Agent i and agent j move to same location
                if agent_i_new_row == agent_j_new_row && agent_i_new_col == agent_j_new_col {
                    return true;
                }

                // Agent i moves to agent j's current location, and agent j stays there
                if agent_i_new_row == agent_j_row
                    && agent_i_new_col == agent_j_col
                    && action_j.get_type() == crate::action::ActionType::NoOp
                {
                    return true;
                }

                // Agent j moves to agent i's current location, and agent i stays there
                if agent_j_new_row == agent_i_row
                    && agent_j_new_col == agent_i_col
                    && action_i.get_type() == crate::action::ActionType::NoOp
                {
                    return true;
                }

                // Agents i and j swap locations
                if agent_i_new_row == agent_j_row
                    && agent_i_new_col == agent_j_col
                    && agent_j_new_row == agent_i_row
                    && agent_j_new_col == agent_i_col
                {
                    return true;
                }

                // Agent i moves to location where agent j is moving a box to
                if action_j.get_type() == crate::action::ActionType::Push
                    && agent_i_new_row == box_j_new_row
                    && agent_i_new_col == box_j_new_col
                {
                    return true;
                }

                // Agent j moves to location where agent i is moving a box to
                if action_i.get_type() == crate::action::ActionType::Push
                    && agent_j_new_row == box_i_new_row
                    && agent_j_new_col == box_i_new_col
                {
                    return true;
                }

                // Agent i moves box to location where agent j is moving to
                if action_i.get_type() == crate::action::ActionType::Push
                    && box_i_new_row == agent_j_new_row
                    && box_i_new_col == agent_j_new_col
                {
                    return true;
                }

                // Agent j moves box to location where agent i is moving to
                if action_j.get_type() == crate::action::ActionType::Push
                    && box_j_new_row == agent_i_new_row
                    && box_j_new_col == agent_i_new_col
                {
                    return true;
                }

                // Agent i and j both move boxes to same location
                if (action_i.get_type() == crate::action::ActionType::Push
                    || action_i.get_type() == crate::action::ActionType::Pull)
                    && (action_j.get_type() == crate::action::ActionType::Push
                        || action_j.get_type() == crate::action::ActionType::Pull)
                    && box_i_new_row == box_j_new_row
                    && box_i_new_col == box_j_new_col
                {
                    return true;
                }

                // Agent i pulls box from location where agent j is moving to
                if action_i.get_type() == crate::action::ActionType::Pull
                    && box_i_row == agent_j_new_row
                    && box_i_col == agent_j_new_col
                {
                    return true;
                }

                // Agent j pulls box from location where agent i is moving to
                if action_j.get_type() == crate::action::ActionType::Pull
                    && box_j_row == agent_i_new_row
                    && box_j_col == agent_i_new_col
                {
                    return true;
                }
            }
        }

        false
    }

    fn cell_is_free(&self, row: usize, col: usize) -> bool {
        // Check if cell is outside the grid
        if row >= self.walls.len() || col >= self.walls[0].len() {
            return false;
        }

        // Check if cell has a wall
        if self.walls[row][col] {
            return false;
        }

        // Check if cell has a box
        if self.boxes[row][col] != '\0' {
            return false;
        }

        // Check if cell has an agent
        self.agent_at(row, col) == '\0'
    }

    fn agent_at(&self, row: usize, col: usize) -> char {
        for (i, (&agent_row, &agent_col)) in self
            .agent_rows
            .iter()
            .zip(self.agent_cols.iter())
            .enumerate()
        {
            if agent_row == row && agent_col == col {
                return (i as u8 + b'0') as char;
            }
        }
        '\0'
    }

    pub fn extract_plan(&self) -> Vec<Vec<Action>> {
        let mut reversed_plan = Vec::new();
        let mut current_state = self;

        while let Some(joint_action) = &current_state.joint_action {
            reversed_plan.push(joint_action.clone());
            current_state = current_state.parent.as_ref().unwrap();
        }

        reversed_plan.reverse();
        reversed_plan
    }

    pub fn calculate_hash(&self) -> u64 {
        let mut hasher = DefaultHasher::new();

        // Hash agent positions
        for (&row, &col) in self.agent_rows.iter().zip(self.agent_cols.iter()) {
            row.hash(&mut hasher);
            col.hash(&mut hasher);
        }

        // Hash box positions
        for row in &self.boxes {
            for &cell in row {
                cell.hash(&mut hasher);
            }
        }

        hasher.finish()
    }
}

impl Clone for State {
    fn clone(&self) -> Self {
        State {
            agent_rows: self.agent_rows.clone(),
            agent_cols: self.agent_cols.clone(),
            agent_colors: Rc::clone(&self.agent_colors),
            walls: Rc::clone(&self.walls),
            boxes: self.boxes.clone(),
            box_colors: Rc::clone(&self.box_colors),
            goals: Rc::clone(&self.goals),
            parent: self.parent.clone(),
            joint_action: self.joint_action.clone(),
            g: self.g,
            hash: self.hash,
        }
    }
}

impl Hash for State {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.hash.hash(state);
    }
}

impl PartialEq for State {
    fn eq(&self, other: &Self) -> bool {
        if self.hash != other.hash {
            return false;
        }

        if self.agent_rows.len() != other.agent_rows.len() {
            return false;
        }

        for i in 0..self.agent_rows.len() {
            if self.agent_rows[i] != other.agent_rows[i]
                || self.agent_cols[i] != other.agent_cols[i]
            {
                return false;
            }
        }

        self.boxes == other.boxes
    }
}

impl Eq for State {}

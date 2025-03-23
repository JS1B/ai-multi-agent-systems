#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Direction {
    N,
    S,
    E,
    W,
}

impl Direction {
    /// Returns the (row, col) offset for the given direction.
    /// North is up (-1, 0), South is down (1, 0), East is right (0, 1), West is left (0, -1).
    pub fn delta(&self) -> (i32, i32) {
        match self {
            Direction::N => (-1, 0),
            Direction::S => (1, 0),
            Direction::E => (0, 1),
            Direction::W => (0, -1),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Action {
    NoOp,
    Move(Direction),
    Push {
        agent: Direction,
        box_move: Direction,
    },
    Pull {
        agent: Direction,
        box_move: Direction,
    },
}

impl Action {
    /// Returns the delta for the agent based on the action.
    /// For NoOp and Move, only the agent moves.
    /// For Push and Pull, the agent’s movement is given by the first direction.
    pub fn agent_delta(&self) -> (i32, i32) {
        match self {
            Action::NoOp => (0, 0),
            Action::Move(dir) => dir.delta(),
            Action::Push { agent, .. } => agent.delta(),
            Action::Pull { agent, .. } => agent.delta(),
        }
    }

    /// Returns the delta for the box based on the action.
    /// For NoOp and Move, the box does not move.
    /// For Push and Pull, the box’s movement is given by the second direction.
    pub fn box_delta(&self) -> (i32, i32) {
        match self {
            Action::NoOp | Action::Move(_) => (0, 0),
            Action::Push { box_move, .. } => box_move.delta(),
            Action::Pull { box_move, .. } => box_move.delta(),
        }
    }
}

impl std::fmt::Display for Direction {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let s = match self {
            Direction::N => "N",
            Direction::S => "S",
            Direction::E => "E",
            Direction::W => "W",
        };
        write!(f, "{}", s)
    }
}

impl std::fmt::Display for Action {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Action::NoOp => write!(f, "NoOp"),
            Action::Move(dir) => write!(f, "Move({})", dir),
            Action::Push { agent, box_move } => write!(f, "Push({},{})", agent, box_move),
            Action::Pull { agent, box_move } => write!(f, "Pull({},{})", agent, box_move),
        }
    }
}

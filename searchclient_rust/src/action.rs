#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ActionType {
    NoOp,
    Move,
    Push,
    Pull,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Action {
    NoOp,
    MoveN,
    MoveS,
    MoveE,
    MoveW,
    PushNN,
    PushNE,
    PushNW,
    PushSS,
    PushSE,
    PushSW,
    PushEE,
    PushEN,
    PushES,
    PushWW,
    PushWN,
    PushWS,
    PullNN,
    PullNE,
    PullNW,
    PullSS,
    PullSE,
    PullSW,
    PullEE,
    PullEN,
    PullES,
    PullWW,
    PullWN,
    PullWS,
}

impl Action {
    pub fn get_name(&self) -> &'static str {
        match self {
            Action::NoOp => "NoOp",
            Action::MoveN => "Move(N)",
            Action::MoveS => "Move(S)",
            Action::MoveE => "Move(E)",
            Action::MoveW => "Move(W)",
            Action::PushNN => "Push(N,N)",
            Action::PushNE => "Push(N,E)",
            Action::PushNW => "Push(N,W)",
            Action::PushSS => "Push(S,S)",
            Action::PushSE => "Push(S,E)",
            Action::PushSW => "Push(S,W)",
            Action::PushEE => "Push(E,E)",
            Action::PushEN => "Push(E,N)",
            Action::PushES => "Push(E,S)",
            Action::PushWW => "Push(W,W)",
            Action::PushWN => "Push(W,N)",
            Action::PushWS => "Push(W,S)",
            Action::PullNN => "Pull(N,N)",
            Action::PullNE => "Pull(N,E)",
            Action::PullNW => "Pull(N,W)",
            Action::PullSS => "Pull(S,S)",
            Action::PullSE => "Pull(S,E)",
            Action::PullSW => "Pull(S,W)",
            Action::PullEE => "Pull(E,E)",
            Action::PullEN => "Pull(E,N)",
            Action::PullES => "Pull(E,S)",
            Action::PullWW => "Pull(W,W)",
            Action::PullWN => "Pull(W,N)",
            Action::PullWS => "Pull(W,S)",
        }
    }

    pub fn get_type(&self) -> ActionType {
        match self {
            Action::NoOp => ActionType::NoOp,
            Action::MoveN | Action::MoveS | Action::MoveE | Action::MoveW => ActionType::Move,
            Action::PushNN
            | Action::PushNE
            | Action::PushNW
            | Action::PushSS
            | Action::PushSE
            | Action::PushSW
            | Action::PushEE
            | Action::PushEN
            | Action::PushES
            | Action::PushWW
            | Action::PushWN
            | Action::PushWS => ActionType::Push,
            Action::PullNN
            | Action::PullNE
            | Action::PullNW
            | Action::PullSS
            | Action::PullSE
            | Action::PullSW
            | Action::PullEE
            | Action::PullEN
            | Action::PullES
            | Action::PullWW
            | Action::PullWN
            | Action::PullWS => ActionType::Pull,
        }
    }

    pub fn agent_row_delta(&self) -> isize {
        match self {
            Action::NoOp => 0,
            Action::MoveN
            | Action::PushNN
            | Action::PushNE
            | Action::PushNW
            | Action::PullNN
            | Action::PullNE
            | Action::PullNW => -1,
            Action::MoveS
            | Action::PushSS
            | Action::PushSE
            | Action::PushSW
            | Action::PullSS
            | Action::PullSE
            | Action::PullSW => 1,
            Action::MoveE
            | Action::PushEE
            | Action::PushEN
            | Action::PushES
            | Action::PullEE
            | Action::PullEN
            | Action::PullES => 0,
            Action::MoveW
            | Action::PushWW
            | Action::PushWN
            | Action::PushWS
            | Action::PullWW
            | Action::PullWN
            | Action::PullWS => 0,
        }
    }

    pub fn agent_col_delta(&self) -> isize {
        match self {
            Action::NoOp => 0,
            Action::MoveN
            | Action::PushNN
            | Action::PushNE
            | Action::PushNW
            | Action::PullNN
            | Action::PullNE
            | Action::PullNW => 0,
            Action::MoveS
            | Action::PushSS
            | Action::PushSE
            | Action::PushSW
            | Action::PullSS
            | Action::PullSE
            | Action::PullSW => 0,
            Action::MoveE
            | Action::PushEE
            | Action::PushEN
            | Action::PushES
            | Action::PullEE
            | Action::PullEN
            | Action::PullES => 1,
            Action::MoveW
            | Action::PushWW
            | Action::PushWN
            | Action::PushWS
            | Action::PullWW
            | Action::PullWN
            | Action::PullWS => -1,
        }
    }

    pub fn box_row_delta(&self) -> isize {
        match self {
            Action::NoOp | Action::MoveN | Action::MoveS | Action::MoveE | Action::MoveW => 0,
            Action::PushNN | Action::PullNN => -1,
            Action::PushSS | Action::PullSS => 1,
            Action::PushNE
            | Action::PushNW
            | Action::PushSE
            | Action::PushSW
            | Action::PushEE
            | Action::PushWW
            | Action::PullNE
            | Action::PullNW
            | Action::PullSE
            | Action::PullSW
            | Action::PullEE
            | Action::PullWW => 0,
            Action::PushEN | Action::PushWN | Action::PullEN | Action::PullWN => -1,
            Action::PushES | Action::PushWS | Action::PullES | Action::PullWS => 1,
        }
    }

    pub fn box_col_delta(&self) -> isize {
        match self {
            Action::NoOp | Action::MoveN | Action::MoveS | Action::MoveE | Action::MoveW => 0,
            Action::PushNN
            | Action::PushSS
            | Action::PushEN
            | Action::PushES
            | Action::PushWN
            | Action::PushWS
            | Action::PullNN
            | Action::PullSS
            | Action::PullEN
            | Action::PullES
            | Action::PullWN
            | Action::PullWS => 0,
            Action::PushNE
            | Action::PushSE
            | Action::PushEE
            | Action::PullNE
            | Action::PullSE
            | Action::PullEE => 1,
            Action::PushNW
            | Action::PushSW
            | Action::PushWW
            | Action::PullNW
            | Action::PullSW
            | Action::PullWW => -1,
        }
    }
}

impl ToString for Action {
    fn to_string(&self) -> String {
        self.get_name().to_string()
    }
}

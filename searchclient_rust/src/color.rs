#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Color {
    Blue,
    Red,
    Cyan,
    Purple,
    Green,
    Orange,
    Pink,
    Grey,
    Lightblue,
    Brown,
}

impl Color {
    pub fn from_string(s: &str) -> Self {
        match s.to_lowercase().as_str() {
            "blue" => Color::Blue,
            "red" => Color::Red,
            "cyan" => Color::Cyan,
            "purple" => Color::Purple,
            "green" => Color::Green,
            "orange" => Color::Orange,
            "pink" => Color::Pink,
            "grey" => Color::Grey,
            "lightblue" => Color::Lightblue,
            "brown" => Color::Brown,
            _ => Color::Blue,
        }
    }
}

extern crate sysinfo;

use sysinfo::System;

pub struct Memory {
    sys: System,
}

impl Memory {
    pub fn new() -> Self {
        let mut sys = System::new_all();
        sys.refresh_memory();
        Self { sys }
    }

    pub fn update(&mut self) {
        self.sys.refresh_memory();
    }

    pub fn string_rep(&mut self) -> String {
        self.update();
        format!(
            "[Used: {:.2} MB, Free: {:.2} MB, Total: {:.2} MB]",
            self.sys.used_memory() as f64 / 1048576.0,
            self.sys.free_memory() as f64 / 1048576.0,
            self.sys.total_memory() as f64 / 1048576.0,
        )
    }
}

use std::process::ExitStatus;

pub fn wait_parent() -> Option<ExitStatus> {
    let mut system = sysinfo::System::new();
    let current_pid = sysinfo::get_current_pid().unwrap();
    system.refresh_processes_specifics(
        sysinfo::ProcessesToUpdate::Some(&[current_pid]),
        false,
        sysinfo::ProcessRefreshKind::nothing(),
    );
    let parent_pid = system.process(current_pid).unwrap().parent().unwrap();
    system.refresh_processes_specifics(
        sysinfo::ProcessesToUpdate::Some(&[parent_pid]),
        false,
        sysinfo::ProcessRefreshKind::nothing(),
    );
    match system.process(parent_pid) {
        Some(parent) => parent.wait(),
        None => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
}

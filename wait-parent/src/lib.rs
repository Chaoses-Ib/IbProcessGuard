use std::process::ExitStatus;

#[cfg(feature = "debug")]
#[doc(hidden)]
#[allow(unused)]
macro_rules! sysinfo_debug {
    ($($x:tt)*) => {{
        eprintln!($($x)*);
    }}
}

#[cfg(not(feature = "debug"))]
#[doc(hidden)]
#[allow(unused)]
macro_rules! sysinfo_debug {
    ($($x:tt)*) => {{}};
}

pub fn wait_parent() -> Option<ExitStatus> {
    let mut system = sysinfo::System::new();
    let current_pid = sysinfo::get_current_pid().unwrap();
    system.refresh_processes_specifics(
        sysinfo::ProcessesToUpdate::Some(&[current_pid]),
        false,
        sysinfo::ProcessRefreshKind::nothing(),
    );
    let parent_pid = system.process(current_pid).unwrap().parent().unwrap();
    sysinfo_debug!("parent_pid: {:?}", parent_pid);
    system.refresh_processes_specifics(
        sysinfo::ProcessesToUpdate::Some(&[parent_pid]),
        false,
        sysinfo::ProcessRefreshKind::nothing(),
    );
    match system.process(parent_pid) {
        Some(parent) => parent.wait(),
        None => {
            sysinfo_debug!("parent process not found: {:?}", parent_pid);
            None
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
}

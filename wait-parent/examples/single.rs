fn main() {
    match std::env::args().nth(1).as_deref() {
        Some("--wait-parent") => {
            // std::thread::sleep(std::time::Duration::from_secs(5));

            println!("Waiting parent...");
            let status = wait_parent::wait_parent();

            println!("Parent exited with {:?}", status);
            // std::fs::write("status.txt", format!("{:?}", status)).unwrap();

            const STATUS_CONTROL_C_EXIT: i32 = 0xC000013Au32 as i32;
            match status.and_then(|s| s.code()) {
                Some(0 | STATUS_CONTROL_C_EXIT) => (),
                Some(_) | None => {
                    // std::process::Command::new(std::env::current_exe().unwrap())
                    //     .spawn()
                    //     .unwrap();

                    std::thread::sleep(std::time::Duration::from_secs(5));
                    println!("Child exiting...");
                }
            }
        }
        Some(_) => unreachable!(),
        None => {
            println!("Spwaning child...");
            std::process::Command::new(std::env::current_exe().unwrap())
                .arg("--wait-parent")
                .spawn()
                .unwrap();

            std::thread::sleep(std::time::Duration::from_secs(3));

            // panic!();
        }
    }
}

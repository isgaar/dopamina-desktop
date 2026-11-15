// Location: src/core/single_instance.rs
// Function: Unix domain socket single-instance IPC delivering dopamine://
// handoff URIs to the running desktop instance without spawning duplicates.
// All Rights Reserved isgaar

use std::fs;
use std::io::{BufRead, BufReader, Write};
use std::os::unix::net::{UnixListener, UnixStream};
use std::path::PathBuf;
use std::sync::mpsc::{self, Receiver};
use std::time::Duration;

pub struct SingleInstanceGuard {
    socket_path: PathBuf,
    rx: Receiver<String>,
}

impl SingleInstanceGuard {
    pub fn try_acquire(initial_uri: Option<&str>) -> Result<Self, ()> {
        let socket_path = socket_file_path();

        if let Ok(mut stream) = UnixStream::connect(&socket_path) {
            let _ = stream.set_write_timeout(Some(Duration::from_millis(500)));
            let payload = initial_uri.unwrap_or("RAISE");
            let _ = writeln!(stream, "{payload}");
            return Err(());
        }

        let _ = fs::remove_file(&socket_path);
        let listener = match UnixListener::bind(&socket_path) {
            Ok(l) => l,
            Err(_) => {
                // Fallback dummy receiver if socket bind fails in restricted sandbox
                let (_tx, rx) = mpsc::channel();
                return Ok(Self { socket_path, rx });
            }
        };

        let (tx, rx) = mpsc::channel();
        std::thread::spawn(move || {
            for stream in listener.incoming().flatten() {
                let mut reader = BufReader::new(stream);
                let mut line = String::new();
                if reader.read_line(&mut line).is_ok() {
                    let msg = line.trim().to_string();
                    if !msg.is_empty() {
                        let _ = tx.send(msg);
                    }
                }
            }
        });

        Ok(Self { socket_path, rx })
    }

    pub fn poll_messages(&self) -> Vec<String> {
        let mut out = Vec::new();
        while let Ok(msg) = self.rx.try_recv() {
            out.push(msg);
        }
        out
    }
}

impl Drop for SingleInstanceGuard {
    fn drop(&mut self) {
        let _ = fs::remove_file(&self.socket_path);
    }
}

fn socket_file_path() -> PathBuf {
    let dir = std::env::var("XDG_RUNTIME_DIR")
        .ok()
        .filter(|s| !s.trim().is_empty())
        .map(PathBuf::from)
        .unwrap_or_else(std::env::temp_dir);
    dir.join("dopamine-desktop.sock")
}


// Location: rust/build.rs
// Function: Generates and compiles the cxx-qt C++ bridge for the Rust core.
// All Rights Reserved isgaar

use cxx_qt_build::CxxQtBuilder;

fn main() {
    CxxQtBuilder::new()
        .qt_module("Qml")
        .files(["src/bridge.rs"])
        .build();
}

// Location: src/session/mod.rs
// Function: Session feature root aggregating handoff parsing, the user model,
// owner-only persistence, auth requests, and the pure Rust session controller.
// All Rights Reserved isgaar

pub mod api;
pub mod controller;
pub mod handoff;
pub mod model;
pub mod store;

pub use controller::{SessionController, SessionEvent};
pub use handoff::{parse_handoff, HandoffKind, HandoffLink};
pub use model::{normalized_base_url, server_host, SessionUser};
pub use store::SessionStore;

// Location: rust/src/session/mod.rs
// Function: Session feature root aggregating handoff parsing, the user model,
// owner-only persistence, auth requests, and the cxx-qt bridge.
// All Rights Reserved isgaar

pub mod api;
pub mod handoff;
pub mod model;
pub mod session_bridge;
pub mod store;

pub use session_bridge::SessionBridgeRust;
pub use handoff::{parse_handoff, HandoffKind, HandoffLink};
pub use model::{normalized_base_url, server_host, SessionUser};
pub use store::SessionStore;

// Location: rust/src/data/remote/mod.rs
// Function: Remote access layer for the Dopamine FastAPI backend.
// All Rights Reserved isgaar

pub mod dopamine_api_client;

pub use dopamine_api_client::{ApiError, DopamineApiClient};

// Location: src/features/mod.rs
// Function: Application feature slices for library catalog, queue/mix,
// playlists/likes, and remote session playback synchronization.
// All Rights Reserved isgaar

pub mod interconnect;
pub mod library;
pub mod playlists;
pub mod queue;

pub use interconnect::{InterconnectAction, SessionPlaybackController};
pub use library::{CollectionGroup, LibraryController};
pub use playlists::PlaylistController;
pub use queue::QueueController;


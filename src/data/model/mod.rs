// Location: src/data/model/mod.rs
// Function: Domain model exports for tracks, playback state, and playlists.
// All Rights Reserved isgaar

pub mod playback;
pub mod track;

pub use playback::{PlaybackState, Playlist};
pub use track::{
    are_tracks_duplicates, compute_fidelity_score, compute_section_letter,
    deduplicate_and_sort_tracks, Track,
};

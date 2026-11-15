// Location: src/audio/mod.rs
// Function: Audio subsystem exports combining Symphonia decoding, DSP playback
// worker, PulseAudio/PipeWire output sink, and AutoMix transition planning.
// All Rights Reserved isgaar

pub mod decoder;
pub mod output;
pub mod player;
pub mod transition;

pub use player::AudioPlayer;
pub use transition::{fold_tempo, is_camelot_compatible, plan_transition, TransitionKind};


// Location: src/features/queue.rs
// Function: Queue controller orchestrating playback queues, shuffle/repeat,
// Mix triangulation refills with deduplication, and affinity feedback.
// All Rights Reserved isgaar

use std::collections::HashSet;
use std::sync::mpsc::{self, Receiver, Sender};
use std::time::Instant;

use crate::core::settings::SettingsData;
use crate::data::model::Track;
use crate::data::remote::DopamineApiClient;

pub struct QueueController {
    pub queue: Vec<Track>,
    pub current_index: Option<usize>,
    pub is_shuffle: bool,
    pub is_repeat: bool,
    pub is_mix_active: bool,
    pub mix_seed_id: i64,
    pub mix_seed_title: String,
    pub history: Vec<i64>,
    is_refilling_mix: bool,
    track_started_at: Option<Instant>,
    mix_rx: Receiver<(i64, Vec<Track>)>,
    mix_tx: Sender<(i64, Vec<Track>)>,
}

impl Default for QueueController {
    fn default() -> Self {
        Self::new()
    }
}

impl QueueController {
    pub fn new() -> Self {
        let (mix_tx, mix_rx) = mpsc::channel();
        Self {
            queue: Vec::new(),
            current_index: None,
            is_shuffle: false,
            is_repeat: false,
            is_mix_active: false,
            mix_seed_id: 0,
            mix_seed_title: String::new(),
            history: Vec::new(),
            is_refilling_mix: false,
            track_started_at: None,
            mix_rx,
            mix_tx,
        }
    }

    pub fn current_track(&self) -> Option<&Track> {
        self.current_index.and_then(|idx| self.queue.get(idx))
    }

    pub fn poll(&mut self, settings: &SettingsData) {
        while let Ok((seed_id, mut candidates)) = self.mix_rx.try_recv() {
            self.is_refilling_mix = false;
            if !self.is_mix_active || seed_id != self.mix_seed_id {
                continue;
            }
            let mut seen_ids: HashSet<i64> = self.queue.iter().map(|t| t.id).collect();
            let mut seen_sigs: HashSet<String> = self
                .queue
                .iter()
                .map(|t| track_signature(&t.title, &t.artist))
                .collect();

            candidates.retain(|t| {
                if t.id <= 0 || seen_ids.contains(&t.id) {
                    return false;
                }
                let sig = track_signature(&t.title, &t.artist);
                if !sig.is_empty() && sig != "|" && seen_sigs.contains(&sig) {
                    return false;
                }
                seen_ids.insert(t.id);
                seen_sigs.insert(sig);
                true
            });

            candidates.sort_by(|a, b| {
                let ka = format!("pair_{seed_id}_{}", a.id);
                let kb = format!("pair_{seed_id}_{}", b.id);
                let sa = settings.mix_affinity.get(&ka).copied().unwrap_or(0.0);
                let sb = settings.mix_affinity.get(&kb).copied().unwrap_or(0.0);
                sb.partial_cmp(&sa).unwrap_or(std::cmp::Ordering::Equal)
            });

            self.queue.extend(candidates);
        }
    }

    pub fn play_from_library(
        &mut self,
        tracks: &[Track],
        index: usize,
        client: Option<&DopamineApiClient>,
        settings: &mut SettingsData,
    ) -> Option<Track> {
        if tracks.is_empty() {
            return None;
        }
        self.record_listen_feedback(client, settings);
        self.queue = tracks.to_vec();
        self.current_index = Some(index.min(self.queue.len() - 1));
        self.is_mix_active = false;
        self.mix_seed_id = 0;
        self.mix_seed_title.clear();
        self.activate_current_track()
    }

    pub fn play_now(
        &mut self,
        track: Track,
        client: Option<&DopamineApiClient>,
        settings: &mut SettingsData,
    ) -> Option<Track> {
        self.record_listen_feedback(client, settings);
        let insert_at = self
            .current_index
            .map(|i| (i + 1).min(self.queue.len()))
            .unwrap_or(0);
        self.queue.insert(insert_at, track);
        self.current_index = Some(insert_at);
        self.activate_current_track()
    }

    pub fn play_next(&mut self, track: Track) -> Option<Track> {
        if self.queue.is_empty() || self.current_index.is_none() {
            self.queue.push(track);
            self.current_index = Some(0);
            return self.activate_current_track();
        }
        let pos = (self.current_index.unwrap() + 1).min(self.queue.len());
        self.queue.insert(pos, track);
        None
    }

    pub fn append_to_queue(&mut self, track: Track) {
        self.queue.push(track);
    }

    pub fn play_at_index(
        &mut self,
        index: usize,
        client: Option<&DopamineApiClient>,
        settings: &mut SettingsData,
    ) -> Option<Track> {
        if index >= self.queue.len() {
            return None;
        }
        self.record_listen_feedback(client, settings);
        self.current_index = Some(index);
        self.activate_current_track()
    }

    pub fn next(
        &mut self,
        client: Option<&DopamineApiClient>,
        settings: &mut SettingsData,
    ) -> Option<Track> {
        if self.queue.is_empty() {
            return None;
        }
        self.record_listen_feedback(client, settings);
        let cur = self.current_index.unwrap_or(0);
        let next_idx = if self.is_shuffle && self.queue.len() > 1 {
            let secs = std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .map(|d| d.subsec_nanos() as usize)
                .unwrap_or(1);
            let mut candidate = secs % self.queue.len();
            if candidate == cur {
                candidate = (candidate + 1) % self.queue.len();
            }
            candidate
        } else if cur + 1 < self.queue.len() {
            cur + 1
        } else if self.is_repeat {
            0
        } else {
            return None;
        };

        self.current_index = Some(next_idx);
        let out = self.activate_current_track();
        if self.is_mix_active {
            self.refill_mix_if_needed(client);
        }
        out
    }

    pub fn previous(
        &mut self,
        client: Option<&DopamineApiClient>,
        settings: &mut SettingsData,
    ) -> Option<Track> {
        let cur = self.current_index?;
        if cur == 0 {
            return None;
        }
        self.record_listen_feedback(client, settings);
        self.current_index = Some(cur - 1);
        self.activate_current_track()
    }

    pub fn clear_queue(&mut self) {
        self.queue.clear();
        self.current_index = None;
        self.is_mix_active = false;
        self.mix_seed_id = 0;
        self.mix_seed_title.clear();
    }

    pub fn start_mix_track(
        &mut self,
        mut seed: Track,
        client: Option<&DopamineApiClient>,
        settings: &mut SettingsData,
    ) -> Option<Track> {
        self.record_listen_feedback(client, settings);
        seed.is_mix = true;
        self.is_mix_active = true;
        self.mix_seed_id = seed.id;
        self.mix_seed_title = seed.title.clone();
        self.queue.clear();
        self.queue.push(seed);
        self.current_index = Some(0);
        let out = self.activate_current_track();
        self.refill_mix_if_needed(client);
        out
    }

    pub fn apply_session_queue(&mut self, tracks: Vec<Track>, current_index: usize) {
        if tracks.is_empty() {
            return;
        }
        self.queue = tracks;
        self.current_index = Some(current_index.min(self.queue.len() - 1));
        self.is_mix_active = false;
        self.mix_seed_id = 0;
        self.mix_seed_title.clear();
        self.track_started_at = Some(Instant::now());
    }

    pub fn refill_mix_if_needed(&mut self, client: Option<&DopamineApiClient>) {
        if !self.is_mix_active || self.is_refilling_mix {
            return;
        }
        let cur = self.current_index.unwrap_or(0);
        let remaining = self.queue.len().saturating_sub(cur);
        if remaining >= 5 {
            return;
        }
        let Some(client) = client.cloned() else {
            return;
        };
        self.is_refilling_mix = true;
        let seed_id = self.mix_seed_id;
        let history = self.history.clone();
        let tx = self.mix_tx.clone();

        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let tracks = rt
                .block_on(client.fetch_mix_next(seed_id, &history, 25))
                .unwrap_or_default();
            let _ = tx.send((seed_id, tracks));
        });
    }

    fn activate_current_track(&mut self) -> Option<Track> {
        let track = self.current_track()?.clone();
        if track.id > 0 {
            self.history.push(track.id);
            if self.history.len() > 50 {
                self.history.remove(0);
            }
        }
        self.track_started_at = Some(Instant::now());
        Some(track)
    }

    fn record_listen_feedback(
        &mut self,
        client: Option<&DopamineApiClient>,
        settings: &mut SettingsData,
    ) {
        let (Some(started), Some(track)) = (self.track_started_at.take(), self.current_track())
        else {
            return;
        };
        if !self.is_mix_active || self.mix_seed_id <= 0 || track.id <= 0 {
            return;
        }
        let played_sec = started.elapsed().as_secs_f64();
        let ratio = if track.duration > 0.0 {
            played_sec / track.duration
        } else {
            0.0
        };
        let (event_type, delta) = if ratio >= 0.60 || played_sec >= 90.0 {
            ("like", 1.0)
        } else if ratio < 0.20 && played_sec < 35.0 {
            ("skip", -1.0)
        } else {
            return;
        };

        let key = format!("pair_{}_{}", self.mix_seed_id, track.id);
        let prev = settings.mix_affinity.get(&key).copied().unwrap_or(0.0);
        settings.mix_affinity.insert(key, prev + delta);

        if let Some(client) = client.cloned() {
            let seed_id = self.mix_seed_id;
            let track_id = track.id;
            let history = self.history.clone();
            std::thread::spawn(move || {
                if let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                    .enable_all()
                    .build()
                {
                    rt.block_on(client.report_mix_feedback(
                        seed_id,
                        track_id,
                        event_type,
                        played_sec,
                        &history,
                    ));
                }
            });
        }
    }
}

fn track_signature(title: &str, artist: &str) -> String {
    let t: String = title
        .to_lowercase()
        .chars()
        .filter(|c| c.is_ascii_alphanumeric())
        .collect();
    let a: String = artist
        .to_lowercase()
        .chars()
        .filter(|c| c.is_ascii_alphanumeric())
        .collect();
    format!("{t}|{a}")
}


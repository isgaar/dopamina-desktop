// Location: src/features/interconnect.rs
// Function: Bidirectional remote playback synchronization controller polling
// mobile/web sessions, resolving shared queues, and dispatching handoff commands.
// All Rights Reserved isgaar

use std::sync::mpsc::{self, Receiver, Sender};
use std::time::{Duration, Instant};

use serde_json::json;

use crate::data::model::{PlaybackState, Track};
use crate::data::remote::DopamineApiClient;

const POLL_INTERVAL: Duration = Duration::from_millis(1200);

#[derive(Debug, Clone)]
pub enum InterconnectAction {
    PauseLocal,
    TakeoverDesktop {
        tracks: Option<Vec<Track>>,
        current_index: usize,
        position_sec: f64,
        auto_play: bool,
    },
    RemoteSeek(f64),
    RemoteVolume(f32),
}

enum InterconnectMessage {
    Polled {
        generation: u64,
        default_state: Option<PlaybackState>,
        mobile_state: Option<PlaybackState>,
    },
    ResolvedQueue {
        generation: u64,
        user_id: String,
        track_id: i64,
        position_sec: f64,
        playing: bool,
        auto_play: bool,
        tracks: Vec<Track>,
    },
}

pub struct SessionPlaybackController {
    pub active: bool,
    pub has_session: bool,
    pub source_kind: String,
    pub is_controlling_android: bool,
    pub remote_playing: bool,
    pub remote_position_sec: f64,
    pub remote_volume_pct: i32,
    pub local_track_id: i64,
    last_poll: Instant,
    last_sync_sent: Instant,
    last_remote_cmd_sent: Instant,
    last_local_play_at: f64,
    last_signature: String,
    generation: u64,
    poll_in_flight: bool,
    tx: Sender<InterconnectMessage>,
    rx: Receiver<InterconnectMessage>,
}

impl Default for SessionPlaybackController {
    fn default() -> Self {
        Self::new()
    }
}

impl SessionPlaybackController {
    pub fn new() -> Self {
        let (tx, rx) = mpsc::channel();
        Self {
            active: false,
            has_session: false,
            source_kind: String::from("desktop"),
            is_controlling_android: false,
            remote_playing: false,
            remote_position_sec: 0.0,
            remote_volume_pct: 85,
            local_track_id: 0,
            last_poll: Instant::now() - POLL_INTERVAL,
            last_sync_sent: Instant::now() - Duration::from_secs(5),
            last_remote_cmd_sent: Instant::now() - Duration::from_secs(5),
            last_local_play_at: 0.0,
            last_signature: String::new(),
            generation: 0,
            poll_in_flight: false,
            tx,
            rx,
        }
    }

    pub fn start(&mut self) {
        self.active = true;
    }

    pub fn stop(&mut self) {
        self.active = false;
        self.generation += 1;
        self.poll_in_flight = false;
        self.is_controlling_android = false;
        self.has_session = false;
        self.last_signature.clear();
    }

    pub fn tick(
        &mut self,
        client: Option<&DopamineApiClient>,
        local_playing: bool,
        local_track: Option<&Track>,
        local_pos_sec: f64,
        local_vol: f32,
        queue_ids: &[i64],
    ) -> Vec<InterconnectAction> {
        let mut actions = Vec::new();

        while let Ok(msg) = self.rx.try_recv() {
            match msg {
                InterconnectMessage::Polled {
                    generation,
                    default_state,
                    mobile_state,
                } => {
                    if generation != self.generation {
                        continue;
                    }
                    self.poll_in_flight = false;
                    self.handle_polled(
                        client,
                        local_playing,
                        default_state,
                        mobile_state,
                        &mut actions,
                    );
                }
                InterconnectMessage::ResolvedQueue {
                    generation,
                    user_id,
                    track_id,
                    position_sec,
                    playing,
                    auto_play,
                    tracks,
                } => {
                    if generation != self.generation || tracks.is_empty() {
                        continue;
                    }
                    let current_index = tracks
                        .iter()
                        .position(|t| t.id == track_id)
                        .unwrap_or(0);
                    let kind = if user_id == "mobile_client" {
                        "mobile"
                    } else {
                        "web"
                    };
                    self.has_session = true;
                    if auto_play {
                        self.source_kind = String::from("desktop");
                        self.is_controlling_android = false;
                        self.remote_playing = false;
                    } else {
                        self.source_kind = kind.to_string();
                        self.is_controlling_android = kind == "mobile";
                        self.remote_playing = playing;
                    }
                    actions.push(InterconnectAction::TakeoverDesktop {
                        tracks: Some(tracks),
                        current_index,
                        position_sec,
                        auto_play,
                    });
                }
            }
        }

        if !self.active {
            return actions;
        }

        if local_playing && self.last_sync_sent.elapsed() >= Duration::from_secs(2) {
            self.last_sync_sent = Instant::now();
            if let Some(track) = local_track {
                self.publish_local_state(client, track.id, local_pos_sec, true, local_vol, queue_ids);
            }
        }

        if !self.poll_in_flight && self.last_poll.elapsed() >= POLL_INTERVAL {
            if let Some(client) = client.cloned() {
                self.last_poll = Instant::now();
                self.poll_in_flight = true;
                let generation = self.generation;
                let tx = self.tx.clone();
                std::thread::spawn(move || {
                    let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                        .enable_all()
                        .build()
                    else {
                        return;
                    };
                    let (def, mob) = rt.block_on(async {
                        let d = client.fetch_playback_state("default").await.ok();
                        let m = client.fetch_playback_state("mobile_client").await.ok();
                        (d, m)
                    });
                    let _ = tx.send(InterconnectMessage::Polled {
                        generation,
                        default_state: def,
                        mobile_state: mob,
                    });
                });
            }
        }

        actions
    }

    pub fn notify_local_play_started(&mut self) {
        self.source_kind = String::from("desktop");
        self.is_controlling_android = false;
        self.remote_playing = false;
        self.last_local_play_at = now_epoch_sec();
    }

    pub fn switch_to_desktop(
        &mut self,
        client: Option<&DopamineApiClient>,
    ) -> InterconnectAction {
        if let Some(client) = client.cloned() {
            std::thread::spawn(move || {
                if let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                    .enable_all()
                    .build()
                {
                    let payload = json!({
                        "user_id": "mobile_client",
                        "is_playing": false,
                        "device_id": "desktop_client",
                    });
                    let _ = rt.block_on(client.publish_playback_state(&payload));
                }
            });
        }
        self.is_controlling_android = false;
        self.remote_playing = false;
        self.source_kind = String::from("desktop");
        self.last_local_play_at = now_epoch_sec();
        InterconnectAction::TakeoverDesktop {
            tracks: None,
            current_index: 0,
            position_sec: self.remote_position_sec,
            auto_play: true,
        }
    }

    pub fn switch_to_android(
        &mut self,
        client: Option<&DopamineApiClient>,
        track_id: i64,
        pos_sec: f64,
        volume: f32,
        queue_ids: &[i64],
    ) -> InterconnectAction {
        self.is_controlling_android = true;
        self.remote_playing = true;
        self.has_session = true;
        self.source_kind = String::from("mobile");
        self.remote_position_sec = pos_sec.max(0.0);
        self.remote_volume_pct = (volume * 100.0).round() as i32;
        self.last_remote_cmd_sent = Instant::now();

        self.send_remote_command(
            client,
            track_id,
            pos_sec,
            true,
            self.remote_volume_pct,
            queue_ids,
            "transfer_to_mobile",
        );
        InterconnectAction::PauseLocal
    }

    pub fn toggle_remote_play(
        &mut self,
        client: Option<&DopamineApiClient>,
        queue_ids: &[i64],
    ) {
        self.remote_playing = !self.remote_playing;
        self.last_remote_cmd_sent = Instant::now();
        self.send_remote_command(
            client,
            self.local_track_id,
            self.remote_position_sec,
            self.remote_playing,
            self.remote_volume_pct,
            queue_ids,
            "remote_cmd_to_mobile",
        );
    }

    pub fn remote_seek(
        &mut self,
        client: Option<&DopamineApiClient>,
        pos_sec: f64,
        queue_ids: &[i64],
    ) {
        self.remote_position_sec = pos_sec.max(0.0);
        self.last_remote_cmd_sent = Instant::now();
        self.send_remote_command(
            client,
            self.local_track_id,
            self.remote_position_sec,
            self.remote_playing,
            self.remote_volume_pct,
            queue_ids,
            "remote_cmd_to_mobile",
        );
    }

    pub fn remote_set_volume(
        &mut self,
        client: Option<&DopamineApiClient>,
        volume_pct: i32,
        queue_ids: &[i64],
    ) {
        self.remote_volume_pct = volume_pct.clamp(0, 100);
        self.last_remote_cmd_sent = Instant::now();
        self.send_remote_command(
            client,
            self.local_track_id,
            self.remote_position_sec,
            self.remote_playing,
            self.remote_volume_pct,
            queue_ids,
            "remote_cmd_to_mobile",
        );
    }

    fn send_remote_command(
        &self,
        client: Option<&DopamineApiClient>,
        track_id: i64,
        pos_sec: f64,
        playing: bool,
        volume_pct: i32,
        queue_ids: &[i64],
        device_id: &str,
    ) {
        let Some(client) = client.cloned() else {
            return;
        };
        if track_id <= 0 {
            return;
        }
        let ids = if queue_ids.is_empty() {
            vec![track_id]
        } else {
            queue_ids.to_vec()
        };
        let dev = device_id.to_string();
        std::thread::spawn(move || {
            if let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            {
                let payload = json!({
                    "user_id": "mobile_client",
                    "track_id": track_id,
                    "position_sec": pos_sec.max(0.0),
                    "is_playing": playing,
                    "active_queue": ids,
                    "volume": (volume_pct.clamp(0, 100) as f64) / 100.0,
                    "device_id": dev,
                });
                let _ = rt.block_on(client.publish_playback_state(&payload));
            }
        });
    }

    fn publish_local_state(
        &self,
        client: Option<&DopamineApiClient>,
        track_id: i64,
        pos_sec: f64,
        playing: bool,
        volume: f32,
        queue_ids: &[i64],
    ) {
        let Some(client) = client.cloned() else {
            return;
        };
        if track_id <= 0 {
            return;
        }
        let ids = if queue_ids.is_empty() {
            vec![track_id]
        } else {
            queue_ids.to_vec()
        };
        std::thread::spawn(move || {
            if let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            {
                let payload = json!({
                    "user_id": "default",
                    "track_id": track_id,
                    "position_sec": pos_sec.max(0.0),
                    "is_playing": playing,
                    "active_queue": ids,
                    "volume": volume.clamp(0.0, 1.0) as f64,
                    "device_id": "desktop_client",
                });
                let _ = rt.block_on(client.publish_playback_state(&payload));
            }
        });
    }

    fn handle_polled(
        &mut self,
        client: Option<&DopamineApiClient>,
        local_playing: bool,
        default_state: Option<PlaybackState>,
        mobile_state: Option<PlaybackState>,
        actions: &mut Vec<InterconnectAction>,
    ) {
        if let Some(def) = &default_state {
            if def.device_id == "remote_cmd_to_desktop" || def.device_id == "transfer_to_desktop" {
                let cmd_vol = def.volume.clamp(0.0, 1.0) as f32;
                actions.push(InterconnectAction::RemoteVolume(cmd_vol));
                if let Some(tid) = def.track_id.filter(|&id| id > 0 && id != self.local_track_id) {
                    self.local_track_id = tid;
                    self.spawn_resolve(
                        client,
                        "default",
                        &def.active_queue,
                        tid,
                        def.position_sec,
                        def.is_playing,
                        def.is_playing,
                    );
                } else {
                    actions.push(InterconnectAction::RemoteSeek(def.position_sec));
                    if def.is_playing && !local_playing {
                        actions.push(InterconnectAction::TakeoverDesktop {
                            tracks: None,
                            current_index: 0,
                            position_sec: def.position_sec,
                            auto_play: true,
                        });
                    } else if !def.is_playing && local_playing {
                        actions.push(InterconnectAction::PauseLocal);
                    }
                }
                return;
            }
            if def.device_id == "mobile_client" && local_playing {
                actions.push(InterconnectAction::PauseLocal);
                self.is_controlling_android = true;
                self.source_kind = String::from("mobile");
            }
        }

        if local_playing {
            if let Some(mob) = &mobile_state {
                if mob.is_playing
                    && (mob.device_id == "mobile_client" || mob.device_id == "transfer_to_mobile")
                    && mob.updated_at > self.last_local_play_at + 1.2
                {
                    actions.push(InterconnectAction::PauseLocal);
                    self.is_controlling_android = true;
                    self.source_kind = String::from("mobile");
                    self.remote_playing = true;
                } else {
                    return;
                }
            } else {
                return;
            }
        }

        if self.last_remote_cmd_sent.elapsed() < Duration::from_millis(1800) {
            return;
        }

        let mut best: Option<(&str, PlaybackState)> = None;
        for (uid, st_opt) in [("default", default_state), ("mobile_client", mobile_state)] {
            let Some(st) = st_opt else { continue };
            if st.active_queue.is_empty() && st.track_id.unwrap_or(0) <= 0 {
                continue;
            }
            if best
                .as_ref()
                .map(|(_, b)| st.updated_at >= b.updated_at)
                .unwrap_or(true)
            {
                best = Some((uid, st));
            }
        }

        if let Some((uid, st)) = best {
            let tid = st.track_id.unwrap_or(0);
            if tid > 0 {
                self.local_track_id = tid;
            }
            self.remote_position_sec = st.position_sec.max(0.0);
            self.remote_volume_pct = (st.volume.clamp(0.0, 1.0) * 100.0).round() as i32;
            if uid == "mobile_client" {
                self.is_controlling_android = true;
                self.source_kind = String::from("mobile");
                self.remote_playing = st.is_playing;
            }
            let ids = if st.active_queue.is_empty() && tid > 0 {
                vec![tid]
            } else {
                st.active_queue
            };
            let sig = format!(
                "{uid}|{tid}|{}",
                ids.iter()
                    .map(|i| i.to_string())
                    .collect::<Vec<_>>()
                    .join(",")
            );
            if sig != self.last_signature {
                self.last_signature = sig;
                self.spawn_resolve(client, uid, &ids, tid, st.position_sec, st.is_playing, false);
            }
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn spawn_resolve(
        &self,
        client: Option<&DopamineApiClient>,
        user_id: &str,
        ids: &[i64],
        track_id: i64,
        position_sec: f64,
        playing: bool,
        auto_play: bool,
    ) {
        let Some(client) = client.cloned() else {
            return;
        };
        if ids.is_empty() {
            return;
        }
        let ids_vec = ids.to_vec();
        let uid = user_id.to_string();
        let generation = self.generation;
        let tx = self.tx.clone();

        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            if let Ok(tracks) = rt.block_on(client.resolve_tracks(&ids_vec)) {
                let _ = tx.send(InterconnectMessage::ResolvedQueue {
                    generation,
                    user_id: uid,
                    track_id,
                    position_sec,
                    playing,
                    auto_play,
                    tracks,
                });
            }
        });
    }
}

fn now_epoch_sec() -> f64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs_f64())
        .unwrap_or(0.0)
}


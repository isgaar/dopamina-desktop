// Location: src/features/playlists.rs
// Function: Playlist and favorites controller managing server playlists, likes,
// offline downloaded filtering, and local JSON cache synchronization.
// All Rights Reserved isgaar

use std::collections::HashSet;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::mpsc::{self, Receiver, Sender};

use crate::core::settings::SettingsData;
use crate::data::model::Playlist;
use crate::data::remote::DopamineApiClient;

enum PlaylistMessage {
    Playlists(Result<Vec<Playlist>, String>),
    Likes(Result<Vec<i64>, String>),
    Mutated(bool),
}

pub struct PlaylistController {
    pub playlists: Vec<Playlist>,
    pub liked_ids: HashSet<i64>,
    pub is_server_online: bool,
    pub is_loading: bool,
    cache_dir: PathBuf,
    tx: Sender<PlaylistMessage>,
    rx: Receiver<PlaylistMessage>,
}

impl Default for PlaylistController {
    fn default() -> Self {
        Self::new()
    }
}

impl PlaylistController {
    pub fn new() -> Self {
        let (tx, rx) = mpsc::channel();
        let cache_dir = default_cache_dir();
        let mut ctrl = Self {
            playlists: Vec::new(),
            liked_ids: HashSet::new(),
            is_server_online: true,
            is_loading: false,
            cache_dir,
            tx,
            rx,
        };
        ctrl.load_cached();
        ctrl
    }

    pub fn poll(&mut self, client: Option<&DopamineApiClient>) {
        let mut should_reload = false;
        while let Ok(msg) = self.rx.try_recv() {
            match msg {
                PlaylistMessage::Playlists(res) => {
                    self.is_loading = false;
                    match res {
                        Ok(list) => {
                            self.is_server_online = true;
                            self.playlists = list;
                            self.save_cached_playlists();
                        }
                        Err(_) => {
                            self.is_server_online = false;
                            self.load_cached();
                        }
                    }
                }
                PlaylistMessage::Likes(res) => match res {
                    Ok(ids) => {
                        self.is_server_online = true;
                        self.liked_ids = ids.into_iter().collect();
                        self.save_cached_likes();
                    }
                    Err(_) => {
                        self.is_server_online = false;
                    }
                },
                PlaylistMessage::Mutated(ok) => {
                    if ok {
                        should_reload = true;
                    }
                }
            }
        }
        if should_reload {
            self.load_all(client);
        }
    }

    pub fn load_all(&mut self, client: Option<&DopamineApiClient>) {
        let Some(client) = client.cloned() else {
            self.load_cached();
            return;
        };
        self.is_loading = true;
        let tx1 = self.tx.clone();
        let client1 = client.clone();
        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let res = rt
                .block_on(client1.fetch_playlists())
                .map_err(|e| e.to_string());
            let _ = tx1.send(PlaylistMessage::Playlists(res));
        });

        let tx2 = self.tx.clone();
        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let res = rt.block_on(client.fetch_likes()).map_err(|e| e.to_string());
            let _ = tx2.send(PlaylistMessage::Likes(res));
        });
    }

    pub fn effective_playlists(&self, settings: &SettingsData) -> Vec<Playlist> {
        if self.is_server_online {
            return self.playlists.clone();
        }
        self.playlists
            .iter()
            .filter(|pl| {
                pl.tracks.iter().any(|t| {
                    settings
                        .downloaded_tracks
                        .get(&t.id)
                        .map(|p| Path::new(p).exists())
                        .unwrap_or(false)
                })
            })
            .cloned()
            .collect()
    }

    pub fn is_liked(&self, track_id: i64) -> bool {
        self.liked_ids.contains(&track_id)
    }

    pub fn toggle_like(&mut self, track_id: i64, client: Option<&DopamineApiClient>) {
        if track_id <= 0 {
            return;
        }
        let next = !self.liked_ids.contains(&track_id);
        if next {
            self.liked_ids.insert(track_id);
        } else {
            self.liked_ids.remove(&track_id);
        }
        self.save_cached_likes();
        let Some(client) = client.cloned() else {
            return;
        };
        std::thread::spawn(move || {
            if let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            {
                let _ = rt.block_on(client.set_like(track_id, next));
            }
        });
    }

    pub fn create_playlist(&self, name: &str, client: Option<&DopamineApiClient>) {
        let trimmed = name.trim().to_string();
        let Some(client) = client.cloned() else {
            return;
        };
        if trimmed.is_empty() {
            return;
        }
        let tx = self.tx.clone();
        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let ok = rt.block_on(client.create_playlist(&trimmed)).is_ok();
            let _ = tx.send(PlaylistMessage::Mutated(ok));
        });
    }

    pub fn delete_playlist(&self, id: i64, client: Option<&DopamineApiClient>) {
        let Some(client) = client.cloned() else {
            return;
        };
        if id <= 0 {
            return;
        }
        let tx = self.tx.clone();
        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let ok = rt.block_on(client.delete_playlist(id)).is_ok();
            let _ = tx.send(PlaylistMessage::Mutated(ok));
        });
    }

    pub fn add_track_to_playlist(
        &self,
        playlist_id: i64,
        track_id: i64,
        client: Option<&DopamineApiClient>,
    ) {
        let Some(client) = client.cloned() else {
            return;
        };
        if playlist_id <= 0 || track_id <= 0 {
            return;
        }
        let tx = self.tx.clone();
        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let ok = rt
                .block_on(client.add_track_to_playlist(playlist_id, track_id))
                .is_ok();
            let _ = tx.send(PlaylistMessage::Mutated(ok));
        });
    }

    fn load_cached(&mut self) {
        if let Ok(raw) = fs::read_to_string(self.cache_dir.join("playlists.json")) {
            if let Ok(list) = serde_json::from_str::<Vec<Playlist>>(&raw) {
                self.playlists = list;
            }
        }
        if let Ok(raw) = fs::read_to_string(self.cache_dir.join("likes.json")) {
            if let Ok(ids) = serde_json::from_str::<Vec<i64>>(&raw) {
                self.liked_ids = ids.into_iter().collect();
            }
        }
    }

    fn save_cached_playlists(&self) {
        let _ = fs::create_dir_all(&self.cache_dir);
        if let Ok(json) = serde_json::to_string(&self.playlists) {
            let _ = fs::write(self.cache_dir.join("playlists.json"), json);
        }
    }

    fn save_cached_likes(&self) {
        let _ = fs::create_dir_all(&self.cache_dir);
        let ids: Vec<i64> = self.liked_ids.iter().copied().collect();
        if let Ok(json) = serde_json::to_string(&ids) {
            let _ = fs::write(self.cache_dir.join("likes.json"), json);
        }
    }
}

fn default_cache_dir() -> PathBuf {
    let base = std::env::var("XDG_CONFIG_HOME")
        .ok()
        .filter(|s| !s.trim().is_empty())
        .map(PathBuf::from)
        .or_else(|| {
            std::env::var("HOME")
                .ok()
                .map(|h| PathBuf::from(h).join(".config"))
        })
        .unwrap_or_else(std::env::temp_dir);
    base.join("isgaar").join("Dopamine").join("cache")
}


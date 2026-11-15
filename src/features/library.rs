// Location: src/features/library.rs
// Function: Library feature controller managing remote catalog pagination,
// recursive local directory scanning, acoustic deduplication, A-Z grouping,
// track downloads, and asynchronous cover-art texture decoding.
// All Rights Reserved isgaar

use std::collections::{BTreeMap, HashMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::mpsc::{self, Receiver, Sender};

use egui::{Color32, ColorImage, Context, TextureHandle, TextureOptions};

use crate::core::settings::SettingsData;
use crate::data::model::{deduplicate_and_sort_tracks, Track};
use crate::data::remote::DopamineApiClient;

#[derive(Debug, Clone)]
pub struct CollectionGroup {
    pub name: String,
    pub subtitle: String,
    pub cover_url: Option<String>,
    pub tracks: Vec<Track>,
}

enum LibraryMessage {
    TracksPage {
        generation: u64,
        reset: bool,
        tracks: Result<Vec<Track>, String>,
    },
    RecentTracks(Vec<Track>),
    LocalScanFinished(Vec<Track>),
    DownloadFinished {
        track_id: i64,
        dest_path: String,
        ok: bool,
    },
    CoverLoaded {
        key: String,
        rgba: Vec<u8>,
        width: usize,
        height: usize,
        dominant_accent: Option<Color32>,
    },
}

pub struct LibraryController {
    pub all_tracks: Vec<Track>,
    pub tracks: Vec<Track>,
    pub recent_tracks: Vec<Track>,
    pub albums: Vec<CollectionGroup>,
    pub artists: Vec<CollectionGroup>,
    pub genres: Vec<CollectionGroup>,
    pub folders: Vec<CollectionGroup>,
    pub search_query: String,
    pub is_loading: bool,
    pub is_local_source: bool,
    pub status_message: String,
    pub page: u32,
    pub has_more: bool,
    pub active_downloads: HashSet<i64>,
    pub covers: HashMap<String, Option<TextureHandle>>,
    pub cover_accents: HashMap<String, Color32>,
    generation: u64,
    tx: Sender<LibraryMessage>,
    rx: Receiver<LibraryMessage>,
}

impl Default for LibraryController {
    fn default() -> Self {
        Self::new()
    }
}

impl LibraryController {
    pub fn new() -> Self {
        let (tx, rx) = mpsc::channel();
        Self {
            all_tracks: Vec::new(),
            tracks: Vec::new(),
            recent_tracks: Vec::new(),
            albums: Vec::new(),
            artists: Vec::new(),
            genres: Vec::new(),
            folders: Vec::new(),
            search_query: String::new(),
            is_loading: false,
            is_local_source: false,
            status_message: String::new(),
            page: 0,
            has_more: false,
            active_downloads: HashSet::new(),
            covers: HashMap::new(),
            cover_accents: HashMap::new(),
            generation: 0,
            tx,
            rx,
        }
    }

    pub fn poll(&mut self, ctx: &Context, settings: &mut SettingsData) -> Option<String> {
        let mut downloaded_notice = None;
        while let Ok(msg) = self.rx.try_recv() {
            match msg {
                LibraryMessage::TracksPage {
                    generation,
                    reset,
                    tracks,
                } => {
                    if generation != self.generation {
                        continue;
                    }
                    self.is_loading = false;
                    match tracks {
                        Ok(batch) => {
                            self.has_more = batch.len() >= 100;
                            if reset {
                                self.all_tracks = batch;
                            } else {
                                self.all_tracks.extend(batch);
                            }
                            self.rebuild_views();
                        }
                        Err(err) => {
                            let lower = err.to_lowercase();
                            self.status_message = if lower.contains("timed out") {
                                String::from("err_conn_timeout")
                            } else if lower.contains("error sending request")
                                || lower.contains("connection refused")
                                || lower.contains("network error")
                            {
                                String::from("err_conn_refused")
                            } else {
                                String::from("err_api_bad_response")
                            };
                        }
                    }
                }
                LibraryMessage::RecentTracks(recent) => {
                    self.recent_tracks = recent;
                }
                LibraryMessage::LocalScanFinished(scanned) => {
                    self.is_loading = false;
                    self.has_more = false;
                    self.all_tracks = scanned;
                    self.rebuild_views();
                }
                LibraryMessage::DownloadFinished {
                    track_id,
                    dest_path,
                    ok,
                } => {
                    self.active_downloads.remove(&track_id);
                    if ok {
                        settings.downloaded_tracks.insert(track_id, dest_path.clone());
                        self.status_message = format!("Descargado: {dest_path}");
                        downloaded_notice = Some(dest_path);
                    } else {
                        self.status_message = String::from("ERR_SRC_FILE_MISSING");
                    }
                }
                LibraryMessage::CoverLoaded {
                    key,
                    rgba,
                    width,
                    height,
                    dominant_accent,
                } => {
                    let image = ColorImage::from_rgba_unmultiplied([width, height], &rgba);
                    let handle =
                        ctx.load_texture(format!("cover_{key}"), image, TextureOptions::LINEAR);
                    self.covers.insert(key.clone(), Some(handle));
                    if let Some(accent) = dominant_accent {
                        self.cover_accents.insert(key, accent);
                    }
                }
            }
        }
        downloaded_notice
    }

    pub fn load_tracks(
        &mut self,
        client: Option<&DopamineApiClient>,
        local_dir: Option<&str>,
        settings: &SettingsData,
        page: u32,
    ) {
        let page = page.max(1);
        if let Some(dir) = local_dir.filter(|d| !d.trim().is_empty()) {
            if page > 1 {
                return;
            }
            self.is_local_source = true;
            self.is_loading = true;
            self.has_more = false;
            self.page = 1;
            self.status_message = String::from("Escaneando...");
            let root = PathBuf::from(dir);
            let skip_nomedia = settings.skip_nomedia;
            let skip_mp4 = settings.skip_mp4;
            let skip_ogg = settings.skip_ogg;
            let tx = self.tx.clone();
            std::thread::spawn(move || {
                let scanned = scan_local_directory(&root, skip_nomedia, skip_mp4, skip_ogg);
                let _ = tx.send(LibraryMessage::LocalScanFinished(scanned));
            });
            return;
        }

        self.is_local_source = false;
        let Some(client) = client else {
            return;
        };
        if page > 1 && (self.is_loading || !self.has_more) {
            return;
        }
        let reset = page == 1;
        if reset {
            self.generation += 1;
            self.page = 1;
        } else {
            self.page = page;
        }
        let generation = self.generation;
        let query = self.search_query.clone();
        let client = client.clone();
        let tx = self.tx.clone();
        self.is_loading = true;

        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            let res = rt
                .block_on(client.fetch_tracks_page(page, 100, &query))
                .map_err(|e| e.to_string());
            let _ = tx.send(LibraryMessage::TracksPage {
                generation,
                reset,
                tracks: res,
            });
        });
    }

    pub fn load_recent(&self, client: Option<&DopamineApiClient>) {
        let Some(client) = client.cloned() else {
            return;
        };
        let tx = self.tx.clone();
        std::thread::spawn(move || {
            let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            else {
                return;
            };
            if let Ok(list) = rt.block_on(client.fetch_recent_tracks(20)) {
                let _ = tx.send(LibraryMessage::RecentTracks(list));
            }
        });
    }

    pub fn set_search_query(
        &mut self,
        query: &str,
        client: Option<&DopamineApiClient>,
        local_dir: Option<&str>,
        settings: &SettingsData,
    ) {
        if self.search_query == query {
            return;
        }
        self.search_query = query.to_string();
        if self.is_local_source {
            self.rebuild_views();
        } else {
            self.load_tracks(client, local_dir, settings, 1);
        }
    }

    pub fn rebuild_views(&mut self) {
        let q = self.search_query.trim().to_lowercase();
        let mut filtered: Vec<Track> = if q.is_empty() {
            self.all_tracks.clone()
        } else {
            self.all_tracks
                .iter()
                .filter(|t| {
                    t.title.to_lowercase().contains(&q)
                        || t.artist.to_lowercase().contains(&q)
                        || t.album.to_lowercase().contains(&q)
                        || t.genre.to_lowercase().contains(&q)
                })
                .cloned()
                .collect()
        };
        deduplicate_and_sort_tracks(&mut filtered);
        self.tracks = filtered;
        self.status_message = format!("{} canciones", self.tracks.len());

        let mut album_map: BTreeMap<String, Vec<Track>> = BTreeMap::new();
        let mut artist_map: BTreeMap<String, Vec<Track>> = BTreeMap::new();
        let mut genre_map: BTreeMap<String, Vec<Track>> = BTreeMap::new();
        let mut folder_map: BTreeMap<String, Vec<Track>> = BTreeMap::new();

        for t in &self.tracks {
            let alb = if t.album.trim().is_empty() {
                String::from("Album")
            } else {
                t.album.clone()
            };
            album_map.entry(alb).or_default().push(t.clone());

            let art = if t.artist.trim().is_empty() {
                String::from("Artista")
            } else {
                t.artist.clone()
            };
            artist_map.entry(art).or_default().push(t.clone());

            let gen = if t.genre.trim().is_empty() {
                String::from("Sin género")
            } else {
                t.genre.clone()
            };
            genre_map.entry(gen).or_default().push(t.clone());

            let folder = Path::new(&t.path)
                .parent()
                .and_then(|p| p.file_name())
                .and_then(|s| s.to_str())
                .unwrap_or("Biblioteca")
                .to_string();
            folder_map.entry(folder).or_default().push(t.clone());
        }

        self.albums = album_map
            .into_iter()
            .map(|(name, tracks)| CollectionGroup {
                subtitle: tracks
                    .first()
                    .map(|t| t.artist.clone())
                    .unwrap_or_default(),
                cover_url: tracks.iter().find_map(|t| t.cover_url.clone()),
                name,
                tracks,
            })
            .collect();

        self.artists = artist_map
            .into_iter()
            .map(|(name, tracks)| CollectionGroup {
                subtitle: format!("{} pistas", tracks.len()),
                cover_url: tracks.iter().find_map(|t| t.cover_url.clone()),
                name,
                tracks,
            })
            .collect();

        self.genres = genre_map
            .into_iter()
            .map(|(name, tracks)| CollectionGroup {
                subtitle: format!("{} pistas", tracks.len()),
                cover_url: tracks.iter().find_map(|t| t.cover_url.clone()),
                name,
                tracks,
            })
            .collect();

        self.folders = folder_map
            .into_iter()
            .map(|(name, tracks)| CollectionGroup {
                subtitle: format!("{} archivos", tracks.len()),
                cover_url: None,
                name,
                tracks,
            })
            .collect();
    }

    pub fn total_duration_hours(&self) -> f64 {
        let secs: f64 = self.tracks.iter().map(|t| t.duration.max(0.0)).sum();
        let hours = (secs / 3600.0).max(0.0);
        if hours < 0.05 { 0.0 } else { hours }
    }

    pub fn request_cover(
        &mut self,
        cover_url: Option<&str>,
        client: Option<&DopamineApiClient>,
    ) -> Option<&TextureHandle> {
        let url = cover_url?.trim();
        if url.is_empty() {
            return None;
        }
        if self.covers.contains_key(url) {
            return self.covers.get(url).and_then(|opt| opt.as_ref());
        }
        self.covers.insert(url.to_string(), None);

        let url_owned = url.to_string();
        let client_opt = client.cloned();
        let tx = self.tx.clone();
        std::thread::spawn(move || {
            let bytes = if url_owned.starts_with("http://") || url_owned.starts_with("https://") {
                let Some(client) = client_opt else {
                    return;
                };
                let Ok(rt) = tokio::runtime::Builder::new_current_thread()
                    .enable_all()
                    .build()
                else {
                    return;
                };
                match rt.block_on(client.fetch_bytes(&url_owned)) {
                    Ok(b) => b,
                    Err(_) => return,
                }
            } else {
                match fs::read(&url_owned) {
                    Ok(b) => b,
                    Err(_) => return,
                }
            };

            let Ok(img) = image::load_from_memory(&bytes) else {
                return;
            };
            let resized = img.thumbnail(256, 256).to_rgba8();
            let (width, height) = (resized.width() as usize, resized.height() as usize);
            let raw = resized.into_raw();
            let accent = extract_dominant_accent(&raw);
            let _ = tx.send(LibraryMessage::CoverLoaded {
                key: url_owned,
                rgba: raw,
                width,
                height,
                dominant_accent: accent,
            });
        });
        None
    }

    pub fn download_track(
        &mut self,
        track: &Track,
        client: Option<&DopamineApiClient>,
        settings: &SettingsData,
    ) {
        if track.id <= 0 || self.active_downloads.contains(&track.id) {
            return;
        }
        if let Some(existing) = settings.downloaded_tracks.get(&track.id) {
            if Path::new(existing).exists() {
                self.status_message = String::from("La pista ya se encuentra descargada");
                return;
            }
        }
        let dl_dir = download_dir();
        let _ = fs::create_dir_all(&dl_dir);
        let safe_title: String = track
            .title
            .trim()
            .chars()
            .map(|c| match c {
                '\\' | '/' | ':' | '*' | '?' | '"' | '<' | '>' | '|' => '_',
                other => other,
            })
            .collect();
        let ext = track.format.trim().to_ascii_lowercase();
        let ext = if ext.is_empty() { "flac" } else { ext.as_str() };
        let dest = dl_dir.join(format!("{safe_title}.{ext}"));
        let dest_str = dest.to_string_lossy().to_string();

        self.active_downloads.insert(track.id);
        self.status_message = format!("Descargando {}...", track.title);

        let track_id = track.id;
        let source = if !track.stream_url.is_empty() {
            track.stream_url.clone()
        } else {
            track.path.clone()
        };
        let client_opt = client.cloned();
        let tx = self.tx.clone();

        std::thread::spawn(move || {
            let ok = if source.starts_with("http://") || source.starts_with("https://") {
                if let (Some(client), Ok(rt)) = (
                    client_opt,
                    tokio::runtime::Builder::new_current_thread()
                        .enable_all()
                        .build(),
                ) {
                    match rt.block_on(client.fetch_bytes(&source)) {
                        Ok(bytes) => fs::write(&dest, &bytes).is_ok(),
                        Err(_) => false,
                    }
                } else {
                    false
                }
            } else {
                let local = source.strip_prefix("file://").unwrap_or(&source);
                fs::copy(local, &dest).is_ok()
            };
            let _ = tx.send(LibraryMessage::DownloadFinished {
                track_id,
                dest_path: dest_str,
                ok,
            });
        });
    }
}

fn download_dir() -> PathBuf {
    if let Ok(home) = std::env::var("HOME") {
        let dl = PathBuf::from(&home).join("Downloads");
        if dl.exists() {
            return dl;
        }
        return PathBuf::from(home);
    }
    std::env::temp_dir()
}

fn scan_local_directory(
    root: &Path,
    skip_nomedia: bool,
    skip_mp4: bool,
    skip_ogg: bool,
) -> Vec<Track> {
    let mut tracks = Vec::new();
    let mut stack = vec![root.to_path_buf()];
    let mut next_id = 1_000_000i64;

    while let Some(dir) = stack.pop() {
        if skip_nomedia && dir.join(".nomedia").exists() {
            continue;
        }
        let Ok(entries) = fs::read_dir(&dir) else {
            continue;
        };
        let local_cover = ["cover.jpg", "cover.png", "folder.jpg", "folder.png"]
            .iter()
            .map(|name| dir.join(name))
            .find(|p| p.exists())
            .map(|p| p.to_string_lossy().to_string());

        for entry in entries.flatten() {
            let path = entry.path();
            let name = entry.file_name().to_string_lossy().to_string();
            if skip_nomedia && name.starts_with('.') {
                continue;
            }
            if path.is_dir() {
                stack.push(path);
                continue;
            }
            let Some(ext) = path
                .extension()
                .and_then(|s| s.to_str())
                .map(|s| s.to_ascii_lowercase())
            else {
                continue;
            };
            if skip_mp4 && (ext == "mp4" || ext == "m4a") {
                continue;
            }
            if skip_ogg && (ext == "ogg" || ext == "oga") {
                continue;
            }
            if !matches!(
                ext.as_str(),
                "flac" | "mp3" | "wav" | "ogg" | "opus" | "m4a" | "aac" | "alac" | "aiff"
            ) {
                continue;
            }

            let title = path
                .file_stem()
                .and_then(|s| s.to_str())
                .unwrap_or("Pista")
                .to_string();
            let album = dir
                .file_name()
                .and_then(|s| s.to_str())
                .unwrap_or("Carpeta Local")
                .to_string();
            let artist = dir
                .parent()
                .and_then(|p| p.file_name())
                .and_then(|s| s.to_str())
                .unwrap_or("Biblioteca Local")
                .to_string();
            let full_path = path.to_string_lossy().to_string();

            let mut track = Track {
                id: next_id,
                title,
                artist,
                album,
                duration: 0.0,
                format: ext,
                sample_rate: 44_100,
                bitrate: 0,
                bit_depth: 16,
                cover_url: local_cover.clone(),
                stream_url: full_path.clone(),
                path: full_path,
                has_cover: local_cover.is_some(),
                ..Track::default()
            };
            track.enrich_quality();
            tracks.push(track);
            next_id += 1;
        }
    }
    deduplicate_and_sort_tracks(&mut tracks);
    tracks
}

fn extract_dominant_accent(rgba: &[u8]) -> Option<Color32> {
    let mut best_score = 0.0f32;
    let mut best_rgb = None;
    for px in rgba.chunks_exact(4).step_by(8) {
        let r = px[0] as f32 / 255.0;
        let g = px[1] as f32 / 255.0;
        let b = px[2] as f32 / 255.0;
        let max = r.max(g).max(b);
        let min = r.min(g).min(b);
        let sat = if max > 0.0 { (max - min) / max } else { 0.0 };
        let lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
        if lum > 0.22 && lum < 0.85 && sat > 0.35 {
            let score = sat * (1.0 - (lum - 0.55).abs());
            if score > best_score {
                best_score = score;
                best_rgb = Some(Color32::from_rgb(px[0], px[1], px[2]));
            }
        }
    }
    best_rgb
}

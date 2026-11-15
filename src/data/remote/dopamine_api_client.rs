// Location: src/data/remote/dopamine_api_client.rs
// Function: Asynchronous HTTP client for the Dopamine FastAPI backend,
// covering catalog tracks, mix triangulation, playlists, likes, and remote
// playback state synchronization.
// All Rights Reserved isgaar

use std::fmt;
use std::time::Duration;

use serde_json::{json, Value};

use crate::data::model::{PlaybackState, Playlist, Track};

const CONNECT_TIMEOUT: Duration = Duration::from_secs(5);
const REQUEST_TIMEOUT: Duration = Duration::from_secs(12);

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ApiError {
    Network(String),
    Status(u16),
    InvalidPayload(String),
}

impl fmt::Display for ApiError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Network(message) => write!(formatter, "network error: {message}"),
            Self::Status(code) => write!(formatter, "unexpected HTTP status: {code}"),
            Self::InvalidPayload(message) => write!(formatter, "invalid payload: {message}"),
        }
    }
}

impl std::error::Error for ApiError {}

#[derive(Clone)]
pub struct DopamineApiClient {
    base_url: String,
    auth_token: Option<String>,
    http: reqwest::Client,
}

impl DopamineApiClient {
    pub fn new(base_url: impl Into<String>) -> Self {
        let base_url = base_url.into().trim().trim_end_matches('/').to_string();

        let http = reqwest::Client::builder()
            .connect_timeout(CONNECT_TIMEOUT)
            .timeout(REQUEST_TIMEOUT)
            .build()
            .unwrap_or_default();

        Self {
            base_url,
            auth_token: None,
            http,
        }
    }

    pub fn with_token(mut self, token: impl Into<String>) -> Self {
        let tok = token.into();
        if tok.trim().is_empty() {
            self.auth_token = None;
        } else {
            self.auth_token = Some(tok);
        }
        self
    }

    pub fn base_url(&self) -> &str {
        &self.base_url
    }

    pub async fn check_connection(&self) -> bool {
        if self.base_url.is_empty() {
            return false;
        }
        let url = format!("{}/api/v1/ping", self.base_url);
        if let Ok(resp) = self.http.get(&url).send().await {
            if resp.status().is_success() {
                return true;
            }
        }
        let root_url = format!("{}/", self.base_url);
        match self.http.get(root_url).send().await {
            Ok(response) => response.status().is_success(),
            Err(_) => false,
        }
    }

    pub async fn fetch_tracks(&self, limit: u32) -> Result<Vec<Track>, ApiError> {
        self.fetch_tracks_page(1, limit, "").await
    }

    pub async fn fetch_tracks_page(
        &self,
        page: u32,
        limit: u32,
        query: &str,
    ) -> Result<Vec<Track>, ApiError> {
        let mut path = format!("/api/v1/tracks?page={}&limit={}", page.max(1), limit.max(1));
        if !query.trim().is_empty() {
            let encoded: String = url::form_urlencoded::byte_serialize(query.trim().as_bytes())
                .collect();
            path.push_str(&format!("&q={encoded}"));
        }
        let response = self.get(&path).await?;
        let payload: Value = response
            .json()
            .await
            .map_err(|error| ApiError::InvalidPayload(error.to_string()))?;

        let items = payload
            .as_array()
            .or_else(|| payload.get("items").and_then(Value::as_array))
            .or_else(|| payload.get("tracks").and_then(Value::as_array))
            .ok_or_else(|| {
                ApiError::InvalidPayload(String::from("expected a JSON array of tracks"))
            })?;

        Ok(items
            .iter()
            .filter_map(|item| Track::from_json(item, &self.base_url))
            .collect())
    }

    pub async fn fetch_recent_tracks(&self, limit: u32) -> Result<Vec<Track>, ApiError> {
        let response = self
            .get(&format!("/api/v1/tracks/recent?limit={}", limit.max(1)))
            .await?;
        let payload: Value = response
            .json()
            .await
            .map_err(|error| ApiError::InvalidPayload(error.to_string()))?;
        let items = payload
            .as_array()
            .or_else(|| payload.get("items").and_then(Value::as_array))
            .ok_or_else(|| {
                ApiError::InvalidPayload(String::from("expected a JSON array of recent tracks"))
            })?;
        Ok(items
            .iter()
            .filter_map(|item| Track::from_json(item, &self.base_url))
            .collect())
    }

    pub async fn resolve_tracks(&self, ids: &[i64]) -> Result<Vec<Track>, ApiError> {
        if ids.is_empty() {
            return Ok(Vec::new());
        }
        let joined = ids
            .iter()
            .take(500)
            .map(|id| id.to_string())
            .collect::<Vec<_>>()
            .join(",");
        let response = self
            .get(&format!("/api/v1/tracks/resolve?ids={joined}"))
            .await?;
        let payload: Value = response
            .json()
            .await
            .map_err(|error| ApiError::InvalidPayload(error.to_string()))?;
        let items = payload
            .as_array()
            .or_else(|| payload.get("tracks").and_then(Value::as_array))
            .ok_or_else(|| {
                ApiError::InvalidPayload(String::from("expected resolved tracks array"))
            })?;
        let resolved: Vec<Track> = items
            .iter()
            .filter_map(|item| Track::from_json(item, &self.base_url))
            .collect();
        let mut ordered = Vec::with_capacity(ids.len());
        for &target_id in ids.iter().take(500) {
            if let Some(found) = resolved.iter().find(|t| t.id == target_id) {
                ordered.push(found.clone());
            }
        }
        Ok(ordered)
    }

    pub async fn fetch_mix_next(
        &self,
        seed_track_id: i64,
        history_ids: &[i64],
        limit: u32,
    ) -> Result<Vec<Track>, ApiError> {
        let body = json!({
            "seed_track_id": seed_track_id,
            "history_ids": history_ids,
            "mode": "normal",
            "limit": limit.max(1),
        });
        let response = self.post("/api/v1/mix/next", &body).await?;
        let payload: Value = response
            .json()
            .await
            .map_err(|error| ApiError::InvalidPayload(error.to_string()))?;

        if let Some(tracks_arr) = payload
            .get("tracks")
            .or_else(|| payload.get("pistas"))
            .and_then(Value::as_array)
            .or_else(|| payload.as_array())
        {
            let mut tracks: Vec<Track> = tracks_arr
                .iter()
                .filter_map(|item| Track::from_json(item, &self.base_url))
                .collect();
            for t in &mut tracks {
                t.is_mix = true;
            }
            if !tracks.is_empty() {
                return Ok(tracks);
            }
        }

        if let Some(ids_arr) = payload.get("ids").and_then(Value::as_array) {
            let ids: Vec<i64> = ids_arr.iter().filter_map(Value::as_i64).collect();
            let mut resolved = self.resolve_tracks(&ids).await.unwrap_or_default();
            for t in &mut resolved {
                t.is_mix = true;
            }
            return Ok(resolved);
        }

        Ok(Vec::new())
    }

    pub async fn report_mix_feedback(
        &self,
        seed_track_id: i64,
        track_id: i64,
        event_type: &str,
        played_seconds: f64,
        history_ids: &[i64],
    ) {
        let body = json!({
            "seed_track_id": seed_track_id,
            "track_id": track_id,
            "event_type": event_type,
            "played_seconds": played_seconds,
            "history_ids": history_ids,
        });
        let _ = self.post("/api/v1/mix/feedback", &body).await;
    }

    pub async fn fetch_playlists(&self) -> Result<Vec<Playlist>, ApiError> {
        let response = self.get("/api/v1/playlists").await?;
        let payload: Value = response
            .json()
            .await
            .map_err(|error| ApiError::InvalidPayload(error.to_string()))?;
        let arr = payload
            .as_array()
            .or_else(|| payload.get("playlists").and_then(Value::as_array))
            .ok_or_else(|| ApiError::InvalidPayload(String::from("expected playlists array")))?;
        Ok(arr
            .iter()
            .filter_map(|item| Playlist::from_json(item, &self.base_url))
            .collect())
    }

    pub async fn create_playlist(&self, name: &str) -> Result<(), ApiError> {
        let body = json!({ "nombre": name, "name": name });
        let _ = self.post("/api/v1/playlists", &body).await?;
        Ok(())
    }

    pub async fn delete_playlist(&self, id: i64) -> Result<(), ApiError> {
        let _ = self.delete(&format!("/api/v1/playlists/{id}")).await?;
        Ok(())
    }

    pub async fn add_track_to_playlist(
        &self,
        playlist_id: i64,
        track_id: i64,
    ) -> Result<(), ApiError> {
        let body = json!({ "cancion_id": track_id, "track_id": track_id });
        let _ = self
            .post(&format!("/api/v1/playlists/{playlist_id}/tracks"), &body)
            .await?;
        Ok(())
    }

    pub async fn fetch_likes(&self) -> Result<Vec<i64>, ApiError> {
        let response = self.get("/api/v1/likes").await?;
        let payload: Value = response
            .json()
            .await
            .map_err(|error| ApiError::InvalidPayload(error.to_string()))?;
        let arr = payload
            .as_array()
            .or_else(|| payload.get("likes").and_then(Value::as_array))
            .ok_or_else(|| ApiError::InvalidPayload(String::from("expected likes array")))?;
        Ok(arr
            .iter()
            .filter_map(|item| {
                item.as_i64()
                    .or_else(|| item.get("cancion_id").and_then(Value::as_i64))
                    .or_else(|| item.get("id").and_then(Value::as_i64))
            })
            .collect())
    }

    pub async fn set_like(&self, track_id: i64, liked: bool) -> Result<(), ApiError> {
        if liked {
            let _ = self
                .post(&format!("/api/v1/likes/{track_id}"), &json!({}))
                .await?;
        } else {
            let _ = self.delete(&format!("/api/v1/likes/{track_id}")).await?;
        }
        Ok(())
    }

    pub async fn fetch_playback_state(&self, user_id: &str) -> Result<PlaybackState, ApiError> {
        let response = self
            .get(&format!("/api/v1/player/playback/state?user_id={user_id}"))
            .await?;
        let payload: Value = response
            .json()
            .await
            .map_err(|error| ApiError::InvalidPayload(error.to_string()))?;
        Ok(PlaybackState::from_json(&payload))
    }

    pub async fn publish_playback_state(&self, state: &Value) -> Result<(), ApiError> {
        let _ = self.post("/api/v1/player/playback/state", state).await?;
        Ok(())
    }

    pub async fn fetch_bytes(&self, path_or_url: &str) -> Result<Vec<u8>, ApiError> {
        let url = if path_or_url.starts_with("http://") || path_or_url.starts_with("https://") {
            path_or_url.to_string()
        } else {
            format!("{}{}", self.base_url, path_or_url)
        };
        let mut request = self.http.get(url);
        if let Some(token) = &self.auth_token {
            request = request.bearer_auth(token);
        }
        let response = request
            .send()
            .await
            .map_err(|error| ApiError::Network(error.to_string()))?;
        if !response.status().is_success() {
            return Err(ApiError::Status(response.status().as_u16()));
        }
        let bytes = response
            .bytes()
            .await
            .map_err(|error| ApiError::Network(error.to_string()))?;
        Ok(bytes.to_vec())
    }

    async fn get(&self, path: &str) -> Result<reqwest::Response, ApiError> {
        let url = format!("{}{}", self.base_url, path);
        let mut request = self.http.get(url);
        if let Some(token) = &self.auth_token {
            request = request.bearer_auth(token);
        }

        let response = request
            .send()
            .await
            .map_err(|error| ApiError::Network(error.to_string()))?;

        if !response.status().is_success() {
            return Err(ApiError::Status(response.status().as_u16()));
        }

        Ok(response)
    }

    async fn post(&self, path: &str, body: &Value) -> Result<reqwest::Response, ApiError> {
        let url = format!("{}{}", self.base_url, path);
        let mut request = self.http.post(url).json(body);
        if let Some(token) = &self.auth_token {
            request = request.bearer_auth(token);
        }

        let response = request
            .send()
            .await
            .map_err(|error| ApiError::Network(error.to_string()))?;

        if !response.status().is_success() {
            return Err(ApiError::Status(response.status().as_u16()));
        }

        Ok(response)
    }

    async fn delete(&self, path: &str) -> Result<reqwest::Response, ApiError> {
        let url = format!("{}{}", self.base_url, path);
        let mut request = self.http.delete(url);
        if let Some(token) = &self.auth_token {
            request = request.bearer_auth(token);
        }

        let response = request
            .send()
            .await
            .map_err(|error| ApiError::Network(error.to_string()))?;

        if !response.status().is_success() {
            return Err(ApiError::Status(response.status().as_u16()));
        }

        Ok(response)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalizes_base_url() {
        let client = DopamineApiClient::new(" http://127.0.0.1:8000/ ");
        assert_eq!(client.base_url(), "http://127.0.0.1:8000");
    }
}

// Location: rust/src/data/model/playback.rs
// Function: Playback state snapshot mirroring the Android synchronization
// contract with the Dopamine backend.
// All Rights Reserved isgaar

use serde_json::Value;

#[derive(Debug, Clone, PartialEq)]
pub struct PlaybackState {
    pub user_id: String,
    pub track_id: Option<i64>,
    pub position_sec: f64,
    pub is_playing: bool,
    pub active_queue: Vec<i64>,
    pub volume: f64,
}

impl Default for PlaybackState {
    fn default() -> Self {
        Self {
            user_id: String::from("default"),
            track_id: None,
            position_sec: 0.0,
            is_playing: false,
            active_queue: Vec::new(),
            volume: 1.0,
        }
    }
}

impl PlaybackState {
    pub fn from_json(value: &Value) -> Self {
        let active_queue = value
            .get("active_queue")
            .and_then(Value::as_array)
            .map(|items| items.iter().filter_map(Value::as_i64).collect())
            .unwrap_or_default();

        Self {
            user_id: value
                .get("user_id")
                .and_then(Value::as_str)
                .unwrap_or("default")
                .to_string(),
            track_id: value.get("track_id").and_then(Value::as_i64),
            position_sec: value
                .get("position_sec")
                .and_then(Value::as_f64)
                .unwrap_or(0.0),
            is_playing: value
                .get("is_playing")
                .and_then(Value::as_bool)
                .unwrap_or(false),
            active_queue,
            volume: value.get("volume").and_then(Value::as_f64).unwrap_or(1.0),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn parses_playback_snapshot() {
        let payload = json!({
            "user_id": "ismael",
            "track_id": 38,
            "position_sec": 42.5,
            "is_playing": true,
            "active_queue": [38, 39, 40],
            "volume": 0.8
        });

        let state = PlaybackState::from_json(&payload);

        assert_eq!(state.user_id, "ismael");
        assert_eq!(state.track_id, Some(38));
        assert_eq!(state.active_queue, vec![38, 39, 40]);
        assert!(state.is_playing);
        assert_eq!(state.volume, 0.8);
    }

    #[test]
    fn defaults_to_safe_values() {
        let state = PlaybackState::from_json(&json!({}));

        assert_eq!(state, PlaybackState::default());
    }
}

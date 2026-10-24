// Location: rust/src/data/model/track.rs
// Function: Track domain model mirroring the Android catalog contract,
// including Spanish and English payload aliases from the FastAPI backend.
// All Rights Reserved isgaar

use serde_json::Value;

#[derive(Debug, Clone, PartialEq)]
pub struct Track {
    pub id: i64,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub duration: f64,
    pub format: String,
    pub sample_rate: i32,
    pub bitrate: i32,
    pub bit_depth: i32,
    pub cover_url: Option<String>,
    pub stream_url: String,
    pub album_id: i64,
    pub track_number: i32,
    pub disc_number: i32,
    pub genre: String,
    pub year: i32,
    pub bpm: f64,
    pub musical_key: String,
    pub size_bytes: i64,
    pub cover_hash: String,
    pub cover_mime: String,
    pub has_cover: bool,
    pub has_lyrics: bool,
    pub fidelity_score: i32,
}

impl Track {
    pub fn from_json(value: &Value, base_url: &str) -> Option<Self> {
        let id = integer(value, &["id"], -1);
        if id < 0 {
            return None;
        }

        let format = text(value, &["formato", "format"], "flac");
        let has_cover = has_cover(value);
        let bpm = number(value, &["bpm"], 0.0);

        Some(Self {
            id,
            title: text(value, &["titulo", "title"], "Pista"),
            artist: text(value, &["artista", "artist"], "Artista"),
            album: text(value, &["album"], "Album"),
            duration: number(value, &["duracion", "duration"], 0.0),
            bit_depth: integer(
                value,
                &["bit_depth"],
                if format.eq_ignore_ascii_case("flac") {
                    24
                } else {
                    16
                },
            ) as i32,
            sample_rate: integer(value, &["sample_rate"], 44_100) as i32,
            bitrate: integer(value, &["bitrate"], 0) as i32,
            cover_url: has_cover.then(|| format!("{base_url}/api/v1/covers/{id}")),
            stream_url: format!("{base_url}/api/v1/streams/{id}"),
            album_id: integer(value, &["album_id"], 0),
            track_number: integer(value, &["track_number", "numero_pista"], 0) as i32,
            disc_number: integer(value, &["disc_number", "disco"], 1) as i32,
            genre: text(value, &["genre", "genero"], ""),
            year: integer(value, &["year", "anio"], 0) as i32,
            bpm: if bpm.is_nan() { 0.0 } else { bpm },
            musical_key: text(value, &["musical_key", "tonalidad"], ""),
            size_bytes: integer(value, &["size_bytes", "tamano_bytes"], 0),
            cover_hash: text(value, &["cover_hash"], ""),
            cover_mime: text(value, &["cover_mime"], "image/jpeg"),
            has_cover,
            has_lyrics: flag(value, &["has_lyrics"], false),
            fidelity_score: integer(value, &["fidelity_score"], 0) as i32,
            format,
        })
    }

    pub fn display_name(&self) -> String {
        format!("{} - {}", self.artist, self.title)
    }
}

fn text(value: &Value, keys: &[&str], fallback: &str) -> String {
    keys.iter()
        .find_map(|key| value.get(*key).and_then(Value::as_str))
        .unwrap_or(fallback)
        .to_string()
}

fn integer(value: &Value, keys: &[&str], fallback: i64) -> i64 {
    keys.iter()
        .find_map(|key| {
            let item = value.get(*key)?;
            if let Some(number) = item.as_i64() {
                return Some(number);
            }
            if let Some(number) = item.as_f64() {
                return Some(number as i64);
            }
            item.as_str()?.parse::<i64>().ok()
        })
        .unwrap_or(fallback)
}

fn number(value: &Value, keys: &[&str], fallback: f64) -> f64 {
    keys.iter()
        .find_map(|key| {
            let item = value.get(*key)?;
            if let Some(number) = item.as_f64() {
                return Some(number);
            }
            if let Some(number) = item.as_i64() {
                return Some(number as f64);
            }
            item.as_str()?.parse::<f64>().ok()
        })
        .unwrap_or(fallback)
}

fn flag(value: &Value, keys: &[&str], fallback: bool) -> bool {
    keys.iter()
        .find_map(|key| {
            let item = value.get(*key)?;
            if let Some(flag) = item.as_bool() {
                return Some(flag);
            }
            if let Some(number) = item.as_i64() {
                return Some(number != 0 && number != -1);
            }
            item.as_str()
                .map(|text| text.eq_ignore_ascii_case("true") || text == "1")
        })
        .unwrap_or(fallback)
}

fn has_cover(value: &Value) -> bool {
    if let Some(flag) = value.get("has_cover").and_then(Value::as_bool) {
        return flag;
    }
    if let Some(number) = value.get("tiene_cover").and_then(Value::as_i64) {
        return number != -1;
    }
    true
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn parses_spanish_payload() {
        let payload = json!({
            "id": 38,
            "titulo": "Alice",
            "artista": "Cocteau Twins",
            "album": "Treasure",
            "duracion": 245.4,
            "formato": "flac",
            "sample_rate": 96000,
            "bit_depth": 24,
            "numero_pista": 3,
            "genero": "Dream Pop",
            "anio": 1984,
            "bpm": 98.5,
            "tonalidad": "8A",
            "tiene_cover": 1
        });

        let track = Track::from_json(&payload, "http://127.0.0.1:8000").unwrap();

        assert_eq!(track.id, 38);
        assert_eq!(track.title, "Alice");
        assert_eq!(track.artist, "Cocteau Twins");
        assert_eq!(track.format, "flac");
        assert_eq!(track.track_number, 3);
        assert_eq!(track.bpm, 98.5);
        assert_eq!(track.musical_key, "8A");
        assert!(track.has_cover);
        assert_eq!(
            track.cover_url.as_deref(),
            Some("http://127.0.0.1:8000/api/v1/covers/38")
        );
        assert_eq!(track.stream_url, "http://127.0.0.1:8000/api/v1/streams/38");
    }

    #[test]
    fn parses_english_payload_and_defaults() {
        let payload = json!({
            "id": 7,
            "title": "Sea, Swallow Me",
            "artist": "Cocteau Twins",
            "duration": 201.0,
            "format": "mp3",
            "has_cover": false
        });

        let track = Track::from_json(&payload, "http://localhost:8000").unwrap();

        assert_eq!(track.album, "Album");
        assert_eq!(track.bit_depth, 16);
        assert_eq!(track.sample_rate, 44_100);
        assert_eq!(track.disc_number, 1);
        assert!(!track.has_cover);
        assert!(track.cover_url.is_none());
    }

    #[test]
    fn rejects_payload_without_id() {
        let payload = json!({ "title": "Unknown" });
        assert!(Track::from_json(&payload, "http://localhost:8000").is_none());
    }
}

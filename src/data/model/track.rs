// Location: src/data/model/track.rs
// Function: Track domain model mirroring the Android catalog contract,
// including Spanish/English payload aliases, Hi-Fi fidelity tiers, and
// acoustic deduplication.
// All Rights Reserved isgaar

use serde::{Deserialize, Serialize};
use serde_json::Value;

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
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
    pub path: String,
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
    pub fidelity_tier: String,
    pub quality_details: String,
    pub extra_meta: String,
    pub section: String,
    pub is_mix: bool,
}

impl Default for Track {
    fn default() -> Self {
        Self {
            id: 0,
            title: String::from("Pista"),
            artist: String::from("Artista"),
            album: String::from("Album"),
            duration: 0.0,
            format: String::from("flac"),
            sample_rate: 44_100,
            bitrate: 0,
            bit_depth: 16,
            cover_url: None,
            stream_url: String::new(),
            path: String::new(),
            album_id: 0,
            track_number: 0,
            disc_number: 1,
            genre: String::new(),
            year: 0,
            bpm: 0.0,
            musical_key: String::new(),
            size_bytes: 0,
            cover_hash: String::new(),
            cover_mime: String::from("image/jpeg"),
            has_cover: false,
            has_lyrics: false,
            fidelity_score: 0,
            fidelity_tier: String::from("HIGH"),
            quality_details: String::from("FLAC 16-bit / 44 kHz"),
            extra_meta: String::new(),
            section: String::from("#"),
            is_mix: false,
        }
    }
}

impl Track {
    pub fn from_json(value: &Value, base_url: &str) -> Option<Self> {
        let id = integer(value, &["id", "cancion_id"], -1);
        if id < 0 {
            return None;
        }

        let raw_path = text(value, &["path", "streamUrl", "stream_url"], "");
        let mut format = text(value, &["formato", "format", "codec"], "");
        if format.trim().is_empty() {
            format = infer_format_from_path(&raw_path);
        }
        let has_cover = has_cover(value);
        let bpm = number(value, &["bpm"], 0.0);
        let bpm = if bpm.is_nan() { 0.0 } else { bpm };

        let bit_depth = integer(
            value,
            &["bit_depth", "bitDepth"],
            if format.eq_ignore_ascii_case("flac") {
                24
            } else {
                16
            },
        ) as i32;
        let sample_rate = integer(value, &["sample_rate", "sampleRate"], 44_100) as i32;
        let bitrate = integer(value, &["bitrate"], 0) as i32;
        let genre = text(value, &["genre", "genero"], "");
        let year = integer(value, &["year", "anio"], 0) as i32;
        let musical_key = text(value, &["musical_key", "tonalidad", "camelot"], "");
        let title = text(value, &["titulo", "title"], "Pista");

        let stream_url = if !base_url.is_empty() {
            format!("{base_url}/api/v1/streams/{id}")
        } else {
            raw_path.clone()
        };
        let path = if raw_path.starts_with("http://") || raw_path.starts_with("https://") {
            raw_path
        } else if !raw_path.is_empty() && base_url.is_empty() {
            raw_path
        } else {
            stream_url.clone()
        };

        let cover_url = if let Some(explicit) = value
            .get("coverUrl")
            .or_else(|| value.get("cover_url"))
            .and_then(Value::as_str)
            .filter(|s| !s.trim().is_empty())
        {
            if explicit.starts_with('/') && !base_url.is_empty() {
                Some(format!("{base_url}{explicit}"))
            } else {
                Some(explicit.to_string())
            }
        } else if has_cover && !base_url.is_empty() {
            Some(format!("{base_url}/api/v1/covers/{id}"))
        } else {
            None
        };

        let mut track = Self {
            id,
            title,
            artist: text(value, &["artista", "artist"], "Artista"),
            album: text(value, &["album"], "Album"),
            duration: number(value, &["duracion", "duration"], 0.0),
            bit_depth,
            sample_rate,
            bitrate,
            cover_url,
            stream_url,
            path,
            album_id: integer(value, &["album_id"], 0),
            track_number: integer(value, &["track_number", "numero_pista"], 0) as i32,
            disc_number: integer(value, &["disc_number", "disco"], 1) as i32,
            genre,
            year,
            bpm,
            musical_key,
            size_bytes: integer(value, &["size_bytes", "tamano_bytes"], 0),
            cover_hash: text(value, &["cover_hash"], ""),
            cover_mime: text(value, &["cover_mime"], "image/jpeg"),
            has_cover,
            has_lyrics: flag(value, &["has_lyrics"], false),
            fidelity_score: integer(value, &["fidelity_score"], 0) as i32,
            format,
            fidelity_tier: String::new(),
            quality_details: String::new(),
            extra_meta: String::new(),
            section: String::new(),
            is_mix: flag(value, &["isMix", "is_mix"], false),
        };
        track.enrich_quality();
        Some(track)
    }

    pub fn enrich_quality(&mut self) {
        let fmt_lower = self.format.trim().to_lowercase();
        let fmt_upper = fmt_lower.to_uppercase();

        if self.bit_depth <= 0 {
            self.bit_depth = if fmt_lower == "flac" { 24 } else { 16 };
        }
        if self.sample_rate <= 0 {
            self.sample_rate = 44_100;
        }

        let is_lossless = matches!(
            fmt_lower.as_str(),
            "flac" | "wav" | "alac" | "aiff" | "dsd"
        );
        let is_lossy = matches!(
            fmt_lower.as_str(),
            "mp3" | "aac" | "ogg" | "opus" | "m4a" | "wma"
        );

        self.fidelity_tier = if fmt_lower.contains("adpcm") || self.bit_depth == 4 {
            String::from("LOW")
        } else if is_lossless && (self.bit_depth >= 24 || self.sample_rate > 48_000) {
            String::from("MAX")
        } else if is_lossless {
            String::from("HIGH")
        } else if is_lossy && (1..=219).contains(&self.bitrate) {
            String::from("LOW")
        } else {
            String::from("OK")
        };

        let mut details = fmt_upper;
        if self.bit_depth > 0 {
            details.push_str(&format!(" {}-bit", self.bit_depth));
        }
        if self.sample_rate > 0 {
            details.push_str(&format!(" / {} kHz", self.sample_rate / 1000));
        }
        if self.bitrate > 0 {
            details.push_str(&format!(" / {} kbps", self.bitrate));
        }
        self.quality_details = details;

        let mut meta_parts = Vec::new();
        if !self.genre.trim().is_empty() {
            meta_parts.push(self.genre.trim().to_string());
        }
        if self.year > 0 {
            meta_parts.push(self.year.to_string());
        }
        if self.bpm > 0.0 && self.bpm.is_finite() {
            meta_parts.push(format!("{} BPM", self.bpm as i32));
        }
        if !self.musical_key.trim().is_empty() {
            meta_parts.push(self.musical_key.trim().to_string());
        }
        self.extra_meta = meta_parts.join(" • ");
        self.section = compute_section_letter(&self.title);
        if self.fidelity_score <= 0 {
            self.fidelity_score = compute_fidelity_score(self);
        }
    }

    pub fn display_name(&self) -> String {
        format!("{} - {}", self.artist, self.title)
    }
}

pub fn compute_section_letter(raw_title: &str) -> String {
    let trimmed = raw_title.trim();
    let Some(first) = trimmed.chars().next() else {
        return String::from("#");
    };
    let ascii = fold_diacritic(first).to_ascii_uppercase();
    if ascii.is_ascii_alphabetic() {
        ascii.to_string()
    } else {
        String::from("#")
    }
}

fn fold_diacritic(ch: char) -> char {
    match ch {
        'á' | 'à' | 'ä' | 'â' | 'ã' | 'Á' | 'À' | 'Ä' | 'Â' | 'Ã' => 'A',
        'é' | 'è' | 'ë' | 'ê' | 'É' | 'È' | 'Ë' | 'Ê' => 'E',
        'í' | 'ì' | 'ï' | 'î' | 'Í' | 'Ì' | 'Ï' | 'Î' => 'I',
        'ó' | 'ò' | 'ö' | 'ô' | 'õ' | 'Ó' | 'Ò' | 'Ö' | 'Ô' | 'Õ' => 'O',
        'ú' | 'ù' | 'ü' | 'û' | 'Ú' | 'Ù' | 'Ü' | 'Û' => 'U',
        'ñ' | 'Ñ' => 'N',
        'ç' | 'Ç' => 'C',
        other => other,
    }
}

pub fn compute_fidelity_score(track: &Track) -> i32 {
    let mut score = 0;
    if track.duration > 1.0 {
        score += 1000;
    }
    let fmt = track.format.to_lowercase();
    if fmt.contains("flac") || fmt.contains("wav") || fmt.contains("alac") || fmt.contains("aiff") {
        score += 500;
    } else if fmt.contains("aac") || fmt.contains("m4a") || fmt.contains("opus") {
        score += 300;
    } else if fmt.contains("mp3") {
        score += 200;
    } else {
        score += 100;
    }

    if track.bitrate >= 1411 {
        score += 400;
    } else if track.bitrate >= 320 {
        score += 300;
    } else if track.bitrate >= 256 {
        score += 200;
    } else if track.bitrate >= 192 {
        score += 100;
    } else if track.bitrate > 0 {
        score += 50;
    }

    if track.sample_rate >= 96_000 {
        score += 100;
    } else if track.sample_rate >= 48_000 {
        score += 50;
    } else if track.sample_rate >= 44_100 {
        score += 25;
    }

    if track.bit_depth >= 24 {
        score += 100;
    } else if track.bit_depth >= 16 {
        score += 50;
    }
    score
}

fn extract_base_and_version(title: &str) -> (String, String) {
    let lower = title.trim().to_lowercase();
    let version_keywords = [
        "club mix",
        "radio edit",
        "remix",
        "edit",
        "live",
        "acoustic",
        "instrumental",
        "dub",
        "extended",
        "vip",
        "vocal",
        "acapella",
        "cover",
        "version",
        "session",
        "demo",
        "orchestral",
    ];
    let mut found_ver = String::new();
    for kw in version_keywords {
        if lower.contains(kw) {
            found_ver = kw.to_string();
            break;
        }
    }

    let mut clean = String::new();
    let mut depth = 0i32;
    for ch in lower.chars() {
        if matches!(ch, '(' | '[' | '{') {
            depth += 1;
            continue;
        }
        if matches!(ch, ')' | ']' | '}') {
            depth = (depth - 1).max(0);
            continue;
        }
        if depth == 0 {
            clean.push(ch);
        }
    }
    if let Some(idx) = clean.find(" - ").or_else(|| clean.find(" – ")) {
        clean.truncate(idx);
    }
    let alnum: String = clean
        .chars()
        .filter(|c| c.is_alphanumeric() || c.is_whitespace())
        .collect();
    let base = alnum.split_whitespace().collect::<Vec<_>>().join(" ");
    (if base.is_empty() { lower } else { base }, found_ver)
}

pub fn are_tracks_duplicates(a: &Track, b: &Track) -> bool {
    let art_a: String = a
        .artist
        .chars()
        .filter(|c| c.is_alphanumeric() || c.is_whitespace())
        .collect::<String>()
        .trim()
        .to_lowercase();
    let art_b: String = b
        .artist
        .chars()
        .filter(|c| c.is_alphanumeric() || c.is_whitespace())
        .collect::<String>()
        .trim()
        .to_lowercase();
    if art_a.is_empty() || art_b.is_empty() || art_a != art_b {
        return false;
    }

    let (base_a, ver_a) = extract_base_and_version(&a.title);
    let (base_b, ver_b) = extract_base_and_version(&b.title);
    if base_a != base_b || ver_a != ver_b {
        return false;
    }

    if (a.duration <= 1.0 && b.duration > 1.0)
        || (b.duration <= 1.0 && a.duration > 1.0)
        || (a.duration <= 1.0 && b.duration <= 1.0)
    {
        return true;
    }

    let dur_diff = (a.duration - b.duration).abs();
    let max_dur = a.duration.max(b.duration);
    if dur_diff > 3.0 && (dur_diff / max_dur) > 0.015 {
        return false;
    }

    if a.bpm > 10.0 && b.bpm > 10.0 {
        let bpm_diff = (a.bpm - b.bpm).abs();
        let oct1 = (a.bpm * 2.0 - b.bpm).abs();
        let oct2 = (a.bpm - b.bpm * 2.0).abs();
        if bpm_diff > 1.5 && oct1 > 2.0 && oct2 > 2.0 {
            return false;
        }
    }

    true
}

pub fn deduplicate_and_sort_tracks(tracks: &mut Vec<Track>) {
    for track in tracks.iter_mut() {
        track.enrich_quality();
    }

    let mut deduped: Vec<Track> = Vec::with_capacity(tracks.len());
    for candidate in tracks.drain(..) {
        if let Some(existing) = deduped
            .iter_mut()
            .find(|item| are_tracks_duplicates(&candidate, item))
        {
            if candidate.fidelity_score > existing.fidelity_score {
                *existing = candidate;
            }
        } else {
            deduped.push(candidate);
        }
    }

    deduped.sort_by(|a, b| {
        if a.section != b.section {
            if a.section == "#" {
                return std::cmp::Ordering::Less;
            }
            if b.section == "#" {
                return std::cmp::Ordering::Greater;
            }
            return a.section.cmp(&b.section);
        }
        a.title.to_lowercase().cmp(&b.title.to_lowercase())
    });

    *tracks = deduped;
}

fn infer_format_from_path(path: &str) -> String {
    let lower = path.to_lowercase();
    for ext in ["mp3", "m4a", "aac", "wav", "ogg", "opus", "alac", "aiff", "flac"] {
        if lower.ends_with(&format!(".{ext}")) {
            return String::from(ext);
        }
    }
    String::from("flac")
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
    fn parses_spanish_payload_and_hi_res_tier() {
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
        assert_eq!(track.fidelity_tier, "MAX");
        assert_eq!(track.section, "A");
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
        assert_eq!(track.fidelity_tier, "OK");
        assert!(!track.has_cover);
        assert!(track.cover_url.is_none());
    }

    #[test]
    fn rejects_payload_without_id() {
        let payload = json!({ "title": "Unknown" });
        assert!(Track::from_json(&payload, "http://localhost:8000").is_none());
    }

    #[test]
    fn deduplicates_lower_fidelity_while_preserving_remixes() {
        let mp3 = Track::from_json(
            &json!({
                "id": 1,
                "title": "Lorelei",
                "artist": "Cocteau Twins",
                "duration": 223.0,
                "format": "mp3",
                "bitrate": 128
            }),
            "http://localhost:8000",
        )
        .unwrap();
        let flac = Track::from_json(
            &json!({
                "id": 2,
                "title": "Lorelei (Remastered)",
                "artist": "Cocteau Twins",
                "duration": 223.2,
                "format": "flac",
                "bit_depth": 24,
                "sample_rate": 96000
            }),
            "http://localhost:8000",
        )
        .unwrap();
        let remix = Track::from_json(
            &json!({
                "id": 3,
                "title": "Lorelei (Club Mix)",
                "artist": "Cocteau Twins",
                "duration": 223.0,
                "format": "flac"
            }),
            "http://localhost:8000",
        )
        .unwrap();

        let mut list = vec![mp3, flac, remix];
        deduplicate_and_sort_tracks(&mut list);
        assert_eq!(list.len(), 2);
        assert!(list.iter().any(|t| t.id == 2));
        assert!(list.iter().any(|t| t.id == 3));
    }
}

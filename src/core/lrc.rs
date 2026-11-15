// Location: src/core/lrc.rs
// Function: Synchronized .lrc lyrics parser supporting standard timestamps,
// millisecond resolution, and active line lookup by playback position.
// All Rights Reserved isgaar

#[derive(Debug, Clone, PartialEq)]
pub struct LyricLine {
    pub time_sec: f64,
    pub text: String,
}

pub fn parse_lrc(content: &str) -> Vec<LyricLine> {
    let mut lines = Vec::new();
    for raw in content.lines() {
        let mut rest = raw.trim();
        let mut timestamps = Vec::new();
        while rest.starts_with('[') {
            let Some(end) = rest.find(']') else {
                break;
            };
            let tag = &rest[1..end];
            if let Some(sec) = parse_timestamp(tag) {
                timestamps.push(sec);
                rest = rest[end + 1..].trim_start();
            } else {
                break;
            }
        }
        if !timestamps.is_empty() {
            let lyric = rest.trim().to_string();
            for t in timestamps {
                lines.push(LyricLine {
                    time_sec: t,
                    text: lyric.clone(),
                });
            }
        }
    }
    lines.sort_by(|a, b| {
        a.time_sec
            .partial_cmp(&b.time_sec)
            .unwrap_or(std::cmp::Ordering::Equal)
    });
    lines
}

pub fn active_line_index(lines: &[LyricLine], position_sec: f64) -> Option<usize> {
    if lines.is_empty() {
        return None;
    }
    let mut active = None;
    for (idx, line) in lines.iter().enumerate() {
        if line.time_sec <= position_sec + 0.05 {
            active = Some(idx);
        } else {
            break;
        }
    }
    active
}

fn parse_timestamp(tag: &str) -> Option<f64> {
    let (min_str, sec_str) = tag.split_once(':')?;
    let mins: f64 = min_str.trim().parse().ok()?;
    let secs: f64 = sec_str.trim().parse().ok()?;
    if mins.is_finite() && secs.is_finite() {
        Some(mins * 60.0 + secs)
    } else {
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_and_locates_active_lrc_line() {
        let raw = "[00:05.00]First line\n[00:12.50][00:25.00]Chorus\n";
        let parsed = parse_lrc(raw);
        assert_eq!(parsed.len(), 3);
        assert_eq!(active_line_index(&parsed, 13.0), Some(1));
        assert_eq!(active_line_index(&parsed, 26.0), Some(2));
    }
}


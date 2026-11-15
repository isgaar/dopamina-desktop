// Location: src/audio/transition.rs
// Function: Pure Rust AutoMix DJ transition planner computing tempo folding,
// Camelot harmonic compatibility, and bar-aligned crossfade frames.
// All Rights Reserved isgaar

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TransitionKind {
    Beatmatch,
    Armonico,
    FundidoSuave,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct TransitionPlan {
    pub kind: TransitionKind,
    pub duration_sec: f64,
    pub fade_frames: usize,
    pub bars: u32,
}

pub fn fold_tempo(bpm_a: f64, bpm_b: f64) -> f64 {
    if bpm_a <= 0.0 || bpm_b <= 0.0 {
        return 1.0;
    }
    let mut r = bpm_b / bpm_a;
    while r < 0.75 {
        r *= 2.0;
    }
    while r >= 1.5 {
        r /= 2.0;
    }
    r
}

pub fn is_camelot_compatible(key_a: &str, key_b: &str) -> bool {
    let ka = key_a.trim().to_ascii_uppercase();
    let kb = key_b.trim().to_ascii_uppercase();
    if ka.len() < 2 || kb.len() < 2 {
        return false;
    }
    let mode_a = ka.as_bytes()[ka.len() - 1];
    let mode_b = kb.as_bytes()[kb.len() - 1];
    let Ok(num_a) = ka[..ka.len() - 1].parse::<i32>() else {
        return false;
    };
    let Ok(num_b) = kb[..kb.len() - 1].parse::<i32>() else {
        return false;
    };
    if !(1..=12).contains(&num_a) || !(1..=12).contains(&num_b) {
        return false;
    }
    if num_a == num_b {
        return true;
    }
    if mode_a == mode_b {
        let diff = (num_a - num_b).abs();
        return diff == 1 || diff == 11;
    }
    false
}

pub fn plan_transition(
    bpm_a: f64,
    conf_a: f64,
    key_a: &str,
    bpm_b: f64,
    conf_b: f64,
    key_b: &str,
    default_fade_sec: f64,
    sample_rate: u32,
) -> TransitionPlan {
    let sr = sample_rate.max(8_000) as f64;
    let mut can_beatmatch = false;
    let mut bars = 0u32;

    if conf_a >= 0.6 && conf_b >= 0.6 && bpm_a > 0.0 && bpm_b > 0.0 {
        let r = fold_tempo(bpm_a, bpm_b);
        if (r - 1.0).abs() <= 0.03 && is_camelot_compatible(key_a, key_b) {
            can_beatmatch = true;
            bars = if (r - 1.0).abs() <= 0.015 { 16 } else { 8 };
        }
    }

    if can_beatmatch {
        let frames = (bars as f64 * 4.0 * 60.0 * sr / bpm_a).round().max(0.0) as usize;
        let dur = frames as f64 / sr;
        return TransitionPlan {
            kind: TransitionKind::Beatmatch,
            duration_sec: dur,
            fade_frames: frames,
            bars,
        };
    }

    if is_camelot_compatible(key_a, key_b) {
        let dur = if default_fade_sec > 0.0 {
            default_fade_sec
        } else {
            4.0
        };
        let frames = (dur * sr).round().max(0.0) as usize;
        return TransitionPlan {
            kind: TransitionKind::Armonico,
            duration_sec: dur,
            fade_frames: frames,
            bars: 0,
        };
    }

    let dur = default_fade_sec.max(0.0);
    let frames = (dur * sr).round().max(0.0) as usize;
    TransitionPlan {
        kind: TransitionKind::FundidoSuave,
        duration_sec: dur,
        fade_frames: frames,
        bars: 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn plans_beatmatch_and_harmonic_transitions() {
        let plan = plan_transition(124.0, 0.9, "8A", 125.0, 0.9, "9A", 4.0, 44100);
        assert_eq!(plan.kind, TransitionKind::Beatmatch);
        assert_eq!(plan.bars, 16);
        assert!(plan.fade_frames > 0);

        let soft = plan_transition(90.0, 0.8, "1A", 130.0, 0.8, "6B", 3.0, 44100);
        assert_eq!(soft.kind, TransitionKind::FundidoSuave);
    }
}


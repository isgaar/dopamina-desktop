// Location: dopamina-desktop/rust/src/dsp/effects2.rs
// Function: Native Rust audio effects engine (Tone Pitch Shifter, Echo, AutoWah, Reverb, Flanger).
// All Rights Reserved isgaar

use std::f32::consts::PI;
use crate::dsp::equalizer::soft_clip;

const PITCH_BUF_LEN: usize = 2048;
const PITCH_BUF_MASK: usize = PITCH_BUF_LEN - 1;
const PITCH_GRAIN: f32 = 1024.0;

pub struct Effects2Engine {
    sample_rate: f32,

    // Tone & Speed progress tracking (100 = neutral)
    pub tone_progress: i32,
    pub speed_progress: i32,

    // Dual-grain Hann-windowed pitch shifter state
    pitch_buf_l: Vec<f32>,
    pitch_buf_r: Vec<f32>,
    pitch_write_idx: usize,
    pitch_phase: f32,

    // 1. Echo
    pub is_echo_enabled: bool,
    pub echo_gain: f32,
    echo_buffer_l: Vec<f32>,
    echo_buffer_r: Vec<f32>,
    echo_idx: usize,

    // 2. AutoWah
    pub is_autowah_enabled: bool,
    pub autowah_gain: f32,
    autowah_lfo_phase: f32,
    autowah_low_l: f32,
    autowah_band_l: f32,
    autowah_low_r: f32,
    autowah_band_r: f32,

    // 3. Reverb
    pub is_reverb_enabled: bool,
    pub reverb_gain: f32,
    reverb_combs_l: [Vec<f32>; 4],
    reverb_combs_r: [Vec<f32>; 4],
    reverb_comb_idx: [usize; 4],
    reverb_allpass_l: [Vec<f32>; 2],
    reverb_allpass_r: [Vec<f32>; 2],
    reverb_allpass_idx: [usize; 2],

    // 4. Flanger
    pub is_flanger_enabled: bool,
    flanger_buffer_l: Vec<f32>,
    flanger_buffer_r: Vec<f32>,
    flanger_idx: usize,
    flanger_lfo_phase: f32,
}

impl Effects2Engine {
    pub fn new() -> Self {
        let sample_rate = 44100.0;
        let comb_lengths = [1116, 1188, 1277, 1356];
        let allpass_lengths = [225, 556];

        let mut engine = Self {
            sample_rate,
            tone_progress: 100,
            speed_progress: 100,

            pitch_buf_l: vec![0.0; PITCH_BUF_LEN],
            pitch_buf_r: vec![0.0; PITCH_BUF_LEN],
            pitch_write_idx: 0,
            pitch_phase: 0.0,

            is_echo_enabled: false,
            echo_gain: 0.0,
            echo_buffer_l: vec![0.0; 24000],
            echo_buffer_r: vec![0.0; 24000],
            echo_idx: 0,

            is_autowah_enabled: false,
            autowah_gain: 0.0,
            autowah_lfo_phase: 0.0,
            autowah_low_l: 0.0,
            autowah_band_l: 0.0,
            autowah_low_r: 0.0,
            autowah_band_r: 0.0,

            is_reverb_enabled: false,
            reverb_gain: 0.0,
            reverb_combs_l: [
                vec![0.0; comb_lengths[0]],
                vec![0.0; comb_lengths[1]],
                vec![0.0; comb_lengths[2]],
                vec![0.0; comb_lengths[3]],
            ],
            reverb_combs_r: [
                vec![0.0; comb_lengths[0] + 23],
                vec![0.0; comb_lengths[1] + 23],
                vec![0.0; comb_lengths[2] + 23],
                vec![0.0; comb_lengths[3] + 23],
            ],
            reverb_comb_idx: [0; 4],
            reverb_allpass_l: [
                vec![0.0; allpass_lengths[0]],
                vec![0.0; allpass_lengths[1]],
            ],
            reverb_allpass_r: [
                vec![0.0; allpass_lengths[0] + 17],
                vec![0.0; allpass_lengths[1] + 17],
            ],
            reverb_allpass_idx: [0; 2],

            is_flanger_enabled: false,
            flanger_buffer_l: vec![0.0; 2048],
            flanger_buffer_r: vec![0.0; 2048],
            flanger_idx: 0,
            flanger_lfo_phase: 0.0,
        };
        engine.set_sample_rate(sample_rate);
        engine
    }

    pub fn set_sample_rate(&mut self, sr: f32) {
        self.sample_rate = sr.max(8000.0);
        let scale = (self.sample_rate / 44100.0).clamp(0.25, 4.5);

        let echo_len = ((self.sample_rate * 0.26) as usize).max(1024);
        if self.echo_buffer_l.len() != echo_len {
            self.echo_buffer_l = vec![0.0; echo_len];
            self.echo_buffer_r = vec![0.0; echo_len];
            self.echo_idx = 0;
        }

        let base_combs = [1116usize, 1188, 1277, 1356];
        let spread = ((23.0 * scale) as usize).max(7);
        for i in 0..4 {
            let len_l = ((base_combs[i] as f32 * scale) as usize).max(128);
            let len_r = len_l + spread;
            if self.reverb_combs_l[i].len() != len_l {
                self.reverb_combs_l[i] = vec![0.0; len_l];
                self.reverb_combs_r[i] = vec![0.0; len_r];
                self.reverb_comb_idx[i] = 0;
            }
        }

        let base_ap = [225usize, 556];
        let ap_spread = ((17.0 * scale) as usize).max(5);
        for i in 0..2 {
            let len_l = ((base_ap[i] as f32 * scale) as usize).max(64);
            let len_r = len_l + ap_spread;
            if self.reverb_allpass_l[i].len() != len_l {
                self.reverb_allpass_l[i] = vec![0.0; len_l];
                self.reverb_allpass_r[i] = vec![0.0; len_r];
                self.reverb_allpass_idx[i] = 0;
            }
        }

        let flanger_len = ((self.sample_rate * 0.025) as usize).next_power_of_two().max(512);
        if self.flanger_buffer_l.len() != flanger_len {
            self.flanger_buffer_l = vec![0.0; flanger_len];
            self.flanger_buffer_r = vec![0.0; flanger_len];
            self.flanger_idx = 0;
        }
    }

    #[inline(always)]
    pub fn semitones(&self) -> i32 {
        (((self.tone_progress * 3) / 25) - 12).clamp(-12, 12)
    }

    #[inline(always)]
    pub fn is_active(&self) -> bool {
        self.semitones() != 0
            || (self.is_echo_enabled && self.echo_gain > 0.0)
            || (self.is_autowah_enabled && self.autowah_gain > 0.0)
            || (self.is_reverb_enabled && self.reverb_gain > 0.0)
            || self.is_flanger_enabled
    }

    pub fn set_tone(&mut self, progress: i32) {
        self.tone_progress = progress.clamp(0, 200);
    }

    pub fn set_speed(&mut self, progress: i32) {
        self.speed_progress = progress.clamp(0, 200);
    }

    pub fn set_echo(&mut self, enabled: bool, gain: f32) {
        self.is_echo_enabled = enabled;
        self.echo_gain = gain.clamp(0.0, 100.0);
    }

    pub fn set_autowah(&mut self, enabled: bool, gain: f32) {
        self.is_autowah_enabled = enabled;
        self.autowah_gain = gain.clamp(0.0, 100.0);
    }

    pub fn set_reverb(&mut self, enabled: bool, gain: f32) {
        self.is_reverb_enabled = enabled;
        self.reverb_gain = gain.clamp(0.0, 100.0);
    }

    pub fn set_flanger(&mut self, enabled: bool) {
        self.is_flanger_enabled = enabled;
    }

    pub fn clear_buffers(&mut self) {
        self.pitch_buf_l.fill(0.0);
        self.pitch_buf_r.fill(0.0);
        self.pitch_write_idx = 0;
        self.pitch_phase = 0.0;

        self.echo_buffer_l.fill(0.0);
        self.echo_buffer_r.fill(0.0);
        self.echo_idx = 0;

        self.autowah_lfo_phase = 0.0;
        self.autowah_low_l = 0.0;
        self.autowah_band_l = 0.0;
        self.autowah_low_r = 0.0;
        self.autowah_band_r = 0.0;

        for c in self.reverb_combs_l.iter_mut() { c.fill(0.0); }
        for c in self.reverb_combs_r.iter_mut() { c.fill(0.0); }
        self.reverb_comb_idx = [0; 4];
        for a in self.reverb_allpass_l.iter_mut() { a.fill(0.0); }
        for a in self.reverb_allpass_r.iter_mut() { a.fill(0.0); }
        self.reverb_allpass_idx = [0; 2];

        self.flanger_buffer_l.fill(0.0);
        self.flanger_buffer_r.fill(0.0);
        self.flanger_idx = 0;
        self.flanger_lfo_phase = 0.0;
    }

    pub fn reset(&mut self) {
        self.tone_progress = 100;
        self.speed_progress = 100;
        self.is_echo_enabled = false;
        self.echo_gain = 0.0;
        self.is_autowah_enabled = false;
        self.autowah_gain = 0.0;
        self.is_reverb_enabled = false;
        self.reverb_gain = 0.0;
        self.is_flanger_enabled = false;
        self.clear_buffers();
    }

    #[inline(always)]
    fn read_pitch_tap(&self, delay_samples: f32) -> (f32, f32) {
        let read_pos = (self.pitch_write_idx as f32 + PITCH_BUF_LEN as f32) - delay_samples;
        let idx0 = (read_pos.floor() as usize) & PITCH_BUF_MASK;
        let idx1 = (idx0 + 1) & PITCH_BUF_MASK;
        let frac = read_pos - read_pos.floor();
        let l = self.pitch_buf_l[idx0] * (1.0 - frac) + self.pitch_buf_l[idx1] * frac;
        let r = self.pitch_buf_r[idx0] * (1.0 - frac) + self.pitch_buf_r[idx1] * frac;
        (l, r)
    }

    #[inline(always)]
    pub fn process_stereo(&mut self, mut left: f32, mut right: f32) -> (f32, f32) {
        let mut active = false;

        // 0. TONE (Dual-grain Hann-windowed fractional delay pitch shifter)
        let st = self.semitones();
        if st != 0 {
            active = true;
            self.pitch_buf_l[self.pitch_write_idx] = left;
            self.pitch_buf_r[self.pitch_write_idx] = right;

            let pitch_ratio = 2.0_f32.powf(st as f32 / 12.0);
            let p0 = self.pitch_phase;
            let p1 = (p0 + 0.5).fract();

            let d0 = 1.0 + p0 * PITCH_GRAIN;
            let d1 = 1.0 + p1 * PITCH_GRAIN;

            let (l0, r0) = self.read_pitch_tap(d0);
            let (l1, r1) = self.read_pitch_tap(d1);

            let w0 = 0.5 * (1.0 - (2.0 * PI * p0).cos());
            let w1 = 1.0 - w0;

            left = l0 * w0 + l1 * w1;
            right = r0 * w0 + r1 * w1;

            self.pitch_write_idx = (self.pitch_write_idx + 1) & PITCH_BUF_MASK;
            self.pitch_phase = (self.pitch_phase + (1.0 - pitch_ratio) / PITCH_GRAIN).rem_euclid(1.0);
        }

        // 1. ECHO (Stereo delay line with warm feedback)
        if self.is_echo_enabled && self.echo_gain > 0.0 {
            active = true;
            let wet = (self.echo_gain / 100.0).clamp(0.0, 1.0);
            let dry = 1.0 - wet * 0.38;
            let delayed_l = self.echo_buffer_l[self.echo_idx];
            let delayed_r = self.echo_buffer_r[self.echo_idx];

            let feedback = 0.42;
            self.echo_buffer_l[self.echo_idx] = (left + delayed_l * feedback).clamp(-1.5, 1.5);
            self.echo_buffer_r[self.echo_idx] = (right + delayed_r * feedback).clamp(-1.5, 1.5);

            self.echo_idx = (self.echo_idx + 1) % self.echo_buffer_l.len();
            left = left * dry + delayed_l * wet * 0.85;
            right = right * dry + delayed_r * wet * 0.85;
        }

        // 2. AUTOWAH (State-variable resonant bandpass filter modulated by LFO)
        if self.is_autowah_enabled && self.autowah_gain > 0.0 {
            active = true;
            let lfo_rate = 1.6;
            self.autowah_lfo_phase += 2.0 * PI * lfo_rate / self.sample_rate;
            if self.autowah_lfo_phase >= 2.0 * PI {
                self.autowah_lfo_phase -= 2.0 * PI;
            }
            let lfo = 0.5 * (1.0 + self.autowah_lfo_phase.sin());
            let cutoff = 350.0 + lfo * 1850.0;
            let f = (2.0 * (PI * cutoff / self.sample_rate).sin()).clamp(0.01, 0.85);
            let q_damp = 0.45;

            self.autowah_low_l = (self.autowah_low_l + f * self.autowah_band_l).clamp(-2.0, 2.0);
            let high_l = left - self.autowah_low_l - q_damp * self.autowah_band_l;
            self.autowah_band_l = (self.autowah_band_l + f * high_l).clamp(-2.0, 2.0);

            self.autowah_low_r = (self.autowah_low_r + f * self.autowah_band_r).clamp(-2.0, 2.0);
            let high_r = right - self.autowah_low_r - q_damp * self.autowah_band_r;
            self.autowah_band_r = (self.autowah_band_r + f * high_r).clamp(-2.0, 2.0);

            let mix = (self.autowah_gain / 100.0).clamp(0.0, 1.0);
            left = left * (1.0 - mix * 0.55) + self.autowah_band_l * mix * 1.35;
            right = right * (1.0 - mix * 0.55) + self.autowah_band_r * mix * 1.35;
        }

        // 3. REVERB (Stereo Schroeder Comb + Allpass Network)
        if self.is_reverb_enabled && self.reverb_gain > 0.0 {
            active = true;
            let wet = (self.reverb_gain / 100.0).clamp(0.0, 1.0);
            let dry = 1.0 - wet * 0.32;
            let damp = 0.22;
            let feedback = 0.78;

            let mut comb_sum_l = 0.0;
            let mut comb_sum_r = 0.0;

            for i in 0..4 {
                let idx_l = self.reverb_comb_idx[i] % self.reverb_combs_l[i].len();
                let idx_r = self.reverb_comb_idx[i] % self.reverb_combs_r[i].len();
                let out_l = self.reverb_combs_l[i][idx_l];
                let out_r = self.reverb_combs_r[i][idx_r];

                self.reverb_combs_l[i][idx_l] = (left + out_l * feedback * (1.0 - damp)).clamp(-1.5, 1.5);
                self.reverb_combs_r[i][idx_r] = (right + out_r * feedback * (1.0 - damp)).clamp(-1.5, 1.5);

                self.reverb_comb_idx[i] = self.reverb_comb_idx[i].wrapping_add(1);
                comb_sum_l += out_l;
                comb_sum_r += out_r;
            }

            let mut ap_l = comb_sum_l * 0.25;
            let mut ap_r = comb_sum_r * 0.25;

            for i in 0..2 {
                let idx_l = self.reverb_allpass_idx[i] % self.reverb_allpass_l[i].len();
                let idx_r = self.reverb_allpass_idx[i] % self.reverb_allpass_r[i].len();
                let buf_out_l = self.reverb_allpass_l[i][idx_l];
                let buf_out_r = self.reverb_allpass_r[i][idx_r];

                self.reverb_allpass_l[i][idx_l] = (ap_l + buf_out_l * 0.5).clamp(-1.5, 1.5);
                self.reverb_allpass_r[i][idx_r] = (ap_r + buf_out_r * 0.5).clamp(-1.5, 1.5);

                ap_l = -ap_l + buf_out_l;
                ap_r = -ap_r + buf_out_r;

                self.reverb_allpass_idx[i] = self.reverb_allpass_idx[i].wrapping_add(1);
            }

            left = left * dry + ap_l * wet * 0.85;
            right = right * dry + ap_r * wet * 0.85;
        }

        // 4. FLANGER (Fractional-delay stereo LFO comb filter with feedback)
        if self.is_flanger_enabled {
            active = true;
            self.flanger_lfo_phase += 2.0 * PI * 0.35 / self.sample_rate;
            if self.flanger_lfo_phase >= 2.0 * PI {
                self.flanger_lfo_phase -= 2.0 * PI;
            }
            let lfo_l = 0.5 * (1.0 + self.flanger_lfo_phase.sin());
            let lfo_r = 0.5 * (1.0 + (self.flanger_lfo_phase + PI * 0.5).sin());
            let min_del = self.sample_rate * 0.001;
            let sweep = self.sample_rate * 0.0032;
            let delay_l = min_del + lfo_l * sweep;
            let delay_r = min_del + lfo_r * sweep;
            let buf_len = self.flanger_buffer_l.len();
            let mask = buf_len - 1;

            let pos_l = (self.flanger_idx as f32 + buf_len as f32) - delay_l;
            let idx_l0 = (pos_l.floor() as usize) & mask;
            let idx_l1 = (idx_l0 + 1) & mask;
            let frac_l = pos_l - pos_l.floor();
            let flanged_l = self.flanger_buffer_l[idx_l0] * (1.0 - frac_l) + self.flanger_buffer_l[idx_l1] * frac_l;

            let pos_r = (self.flanger_idx as f32 + buf_len as f32) - delay_r;
            let idx_r0 = (pos_r.floor() as usize) & mask;
            let idx_r1 = (idx_r0 + 1) & mask;
            let frac_r = pos_r - pos_r.floor();
            let flanged_r = self.flanger_buffer_r[idx_r0] * (1.0 - frac_r) + self.flanger_buffer_r[idx_r1] * frac_r;

            self.flanger_buffer_l[self.flanger_idx] = (left + flanged_l * 0.35).clamp(-1.5, 1.5);
            self.flanger_buffer_r[self.flanger_idx] = (right + flanged_r * 0.35).clamp(-1.5, 1.5);

            self.flanger_idx = (self.flanger_idx + 1) & mask;
            left = left * 0.68 + flanged_l * 0.52;
            right = right * 0.68 + flanged_r * 0.52;
        }

        if active {
            left = soft_clip(left);
            right = soft_clip(right);
        }

        (left, right)
    }
}

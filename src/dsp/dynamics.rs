// Location: rust/src/dsp/dynamics.rs
// Function: 24-band spectrum analyzer, RMS volume stabilizer, and equal-power crossfade engine in Rust.
// All Rights Reserved isgaar

use std::f32::consts::PI;

pub const NUM_BANDS: usize = 24;
pub const TAIL_CAPACITY_FRAMES: usize = 48_000;

pub static BAND_FREQUENCIES: [f32; NUM_BANDS] = [
    55.0, 70.0, 90.0, 115.0, 148.0, 190.0, 244.0, 312.0,
    400.0, 512.0, 655.0, 840.0, 1075.0, 1375.0, 1760.0, 2250.0,
    2880.0, 3680.0, 4700.0, 6000.0, 7700.0, 9800.0, 12500.0, 14500.0,
];

pub static BAND_WEIGHTS: [f32; NUM_BANDS] = [
    2.2, 2.0, 1.8, 1.7, 1.6, 1.5, 1.5, 1.6,
    1.7, 1.8, 1.9, 2.1, 2.3, 2.5, 2.8, 3.2,
    3.6, 4.0, 4.5, 5.0, 5.5, 6.0, 6.5, 7.0,
];

/// Sample-accurate equal-power crossfade engine with outgoing tail ring buffer
/// and per-sample outgoing gain slewing (eliminating timer zipper noise).
pub struct CrossfadeEngine {
    pub current_out_env: f32,
    pub target_out_env: f32,
    pub slew_coeff: f32,
    tail_l: Vec<f32>,
    tail_r: Vec<f32>,
    write_pos: usize,
    valid_frames: usize,
    crossfade_read_pos: usize,
    crossfade_rem_frames: usize,
    crossfade_tot_frames: usize,
    has_active_tail: bool,
}

impl CrossfadeEngine {
    pub fn new() -> Self {
        Self {
            current_out_env: 1.0,
            target_out_env: 1.0,
            slew_coeff: 0.0015,
            tail_l: vec![0.0; TAIL_CAPACITY_FRAMES],
            tail_r: vec![0.0; TAIL_CAPACITY_FRAMES],
            write_pos: 0,
            valid_frames: 0,
            crossfade_read_pos: 0,
            crossfade_rem_frames: 0,
            crossfade_tot_frames: 0,
            has_active_tail: false,
        }
    }

    pub fn set_sample_rate(&mut self, rate: f32) {
        let sr = rate.max(8000.0);
        // ~20ms exponential smoothing time constant for outgoing envelope updates
        self.slew_coeff = (1.0 / (sr * 0.020)).clamp(0.0002, 0.02);
    }

    pub fn set_out_target(&mut self, target: f32) {
        let clamped = target.clamp(0.0, 1.0);
        // If jumping back to 1.0 on new track start, snap cleanly
        if clamped >= 0.999 && self.target_out_env < 0.5 {
            self.current_out_env = 1.0;
        }
        self.target_out_env = clamped;
    }

    /// Triggers an equal-power crossfade of `fade_frames` stereo frames,
    /// blending the captured outgoing tail buffer into the incoming stream.
    pub fn trigger_crossfade(&mut self, fade_frames: usize) {
        let tot = fade_frames.max(1);
        self.crossfade_tot_frames = tot;
        self.crossfade_rem_frames = tot;
        self.current_out_env = 1.0;
        self.target_out_env = 1.0;

        if self.valid_frames >= 64 {
            let tail_len = tot.min(self.valid_frames).min(TAIL_CAPACITY_FRAMES);
            self.crossfade_read_pos =
                (self.write_pos + TAIL_CAPACITY_FRAMES - tail_len) % TAIL_CAPACITY_FRAMES;
            self.has_active_tail = true;
        } else {
            self.has_active_tail = false;
        }
    }

    pub fn reset(&mut self) {
        self.current_out_env = 1.0;
        self.target_out_env = 1.0;
        self.write_pos = 0;
        self.valid_frames = 0;
        self.crossfade_rem_frames = 0;
        self.crossfade_tot_frames = 0;
        self.has_active_tail = false;
        self.tail_l.fill(0.0);
        self.tail_r.fill(0.0);
    }

    #[inline(always)]
    pub fn next_out_gain(&mut self, external_env: f32) -> f32 {
        self.target_out_env = external_env.clamp(0.0, 1.0);
        self.current_out_env +=
            self.slew_coeff * (self.target_out_env - self.current_out_env);
        if (self.current_out_env - self.target_out_env).abs() < 1e-4 {
            self.current_out_env = self.target_out_env;
        }
        // Equal-power curve on outgoing envelope when fading out (< 0.999)
        if self.current_out_env < 0.999 {
            (self.current_out_env * PI * 0.5).sin()
        } else {
            1.0
        }
    }

    #[inline(always)]
    pub fn process_frame(
        &mut self,
        in_l: f32,
        in_r: f32,
        external_out_env: f32,
        rem_fade_samples: &mut i32,
        tot_fade_samples: i32,
        channels: usize,
    ) -> (f32, f32) {
        let out_gain = self.next_out_gain(external_out_env);
        let mut l = in_l * out_gain;
        let mut r = in_r * out_gain;

        let step = channels.clamp(1, 2) as i32;
        if *rem_fade_samples > 0 && tot_fade_samples > 0 {
            let p = 1.0 - (*rem_fade_samples as f32 / tot_fade_samples as f32);
            let p_clamped = p.clamp(0.0, 1.0);
            let angle = p_clamped * PI * 0.5;
            let gain_in = angle.sin();
            let gain_tail = angle.cos();

            l *= gain_in;
            r *= gain_in;

            if self.has_active_tail && self.valid_frames > 0 {
                let tl = self.tail_l[self.crossfade_read_pos];
                let tr = self.tail_r[self.crossfade_read_pos];
                l += tl * gain_tail;
                r += tr * gain_tail;
                self.crossfade_read_pos =
                    (self.crossfade_read_pos + 1) % TAIL_CAPACITY_FRAMES;
                self.valid_frames = self.valid_frames.saturating_sub(1);
                if self.valid_frames == 0 {
                    self.has_active_tail = false;
                }
            }

            *rem_fade_samples = (*rem_fade_samples - step).max(0);
            if *rem_fade_samples == 0 {
                self.has_active_tail = false;
            }
        } else {
            // Record processed steady-state signal into tail ring buffer for next transition
            self.tail_l[self.write_pos] = l;
            self.tail_r[self.write_pos] = r;
            self.write_pos = (self.write_pos + 1) % TAIL_CAPACITY_FRAMES;
            if self.valid_frames < TAIL_CAPACITY_FRAMES {
                self.valid_frames += 1;
            }
        }

        (l, r)
    }

    /// Mixes two interleaved float32 streams (outgoing `a` and incoming `b`)
    /// using an equal-power sin/cos law over `progress_start..=progress_end`.
    /// Sums of squares (gain_a^2 + gain_b^2) == 1.0 at every sample frame.
    pub fn mix_streams_equal_power(
        a: &[f32],
        b: &[f32],
        out: &mut [f32],
        channels: usize,
        progress_start: f32,
        progress_end: f32,
    ) {
        let ch = channels.clamp(1, 2);
        let frames = (out.len() / ch).min(a.len() / ch).min(b.len() / ch);
        if frames == 0 {
            return;
        }
        let p0 = progress_start.clamp(0.0, 1.0);
        let p1 = progress_end.clamp(0.0, 1.0);
        let dp = if frames > 1 {
            (p1 - p0) / (frames - 1) as f32
        } else {
            0.0
        };

        for f in 0..frames {
            let p = (p0 + dp * f as f32).clamp(0.0, 1.0);
            let angle = p * PI * 0.5;
            let gain_a = angle.cos();
            let gain_b = angle.sin();
            let base = f * ch;
            for c in 0..ch {
                out[base + c] = a[base + c] * gain_a + b[base + c] * gain_b;
            }
        }
    }
}

pub struct AudioDynamicsProcessor {
    pub raw_magnitudes: [f32; NUM_BANDS],
    pub smoothed_gain: f32,
    pub consecutive_silent_samples: usize,
    pub sample_rate: f32,
}

impl AudioDynamicsProcessor {
    pub fn new() -> Self {
        Self {
            raw_magnitudes: [0.08; NUM_BANDS],
            smoothed_gain: 1.0,
            consecutive_silent_samples: 0,
            sample_rate: 44100.0,
        }
    }

    pub fn set_sample_rate(&mut self, rate: f32) {
        self.sample_rate = rate.max(8000.0);
    }

    pub fn reset(&mut self) {
        self.consecutive_silent_samples = 0;
        self.smoothed_gain = 1.0;
        self.raw_magnitudes = [0.08; NUM_BANDS];
    }

    pub fn check_silence(&mut self, samples: &[i16]) -> bool {
        let step = (samples.len() / 64).max(1);
        let mut max_amp = 0i32;
        let mut i = 0;
        while i < samples.len() {
            let amp = (samples[i] as i32).abs();
            if amp > max_amp {
                max_amp = amp;
            }
            if max_amp > 200 {
                break;
            }
            i += step;
        }
        if max_amp <= 200 {
            self.consecutive_silent_samples += samples.len();
            let min_silent = (self.sample_rate * 0.10) as usize;
            self.consecutive_silent_samples >= min_silent.max(2048)
        } else {
            self.consecutive_silent_samples = 0;
            false
        }
    }

    pub fn check_silence_f32(&mut self, samples: &[f32]) -> bool {
        let step = (samples.len() / 64).max(1);
        let threshold = 200.0 / 32768.0;
        let mut max_amp = 0.0f32;
        let mut i = 0;
        while i < samples.len() {
            let amp = samples[i].abs();
            if amp > max_amp {
                max_amp = amp;
            }
            if max_amp > threshold {
                break;
            }
            i += step;
        }
        if max_amp <= threshold {
            self.consecutive_silent_samples += samples.len();
            let min_silent = (self.sample_rate * 0.10) as usize;
            self.consecutive_silent_samples >= min_silent.max(2048)
        } else {
            self.consecutive_silent_samples = 0;
            false
        }
    }

    pub fn compute_stabilizer_gain(&mut self, samples: &[i16], enabled: bool) -> (bool, f32) {
        if !enabled || samples.is_empty() {
            if self.smoothed_gain <= 1.01 {
                self.smoothed_gain = 1.0;
            } else {
                self.smoothed_gain = self.smoothed_gain * 0.94 + 0.06;
            }
            return (false, self.smoothed_gain);
        }

        let step = (samples.len() / 128).max(1);
        let mut sum_squares = 0.0f64;
        let mut measured = 0usize;
        let mut i = 0;
        while i < samples.len() {
            let s = samples[i] as f64 / 32768.0;
            sum_squares += s * s;
            measured += 1;
            i += step;
        }

        self.update_stabilizer_from_rms((sum_squares / measured.max(1) as f64).sqrt() as f32)
    }

    pub fn compute_stabilizer_gain_f32(&mut self, samples: &[f32], enabled: bool) -> (bool, f32) {
        if !enabled || samples.is_empty() {
            if self.smoothed_gain <= 1.01 {
                self.smoothed_gain = 1.0;
            } else {
                self.smoothed_gain = self.smoothed_gain * 0.94 + 0.06;
            }
            return (false, self.smoothed_gain);
        }

        let step = (samples.len() / 128).max(1);
        let mut sum_squares = 0.0f64;
        let mut measured = 0usize;
        let mut i = 0;
        while i < samples.len() {
            let s = samples[i] as f64;
            sum_squares += s * s;
            measured += 1;
            i += step;
        }

        self.update_stabilizer_from_rms((sum_squares / measured.max(1) as f64).sqrt() as f32)
    }

    fn update_stabilizer_from_rms(&mut self, rms: f32) -> (bool, f32) {
        let current_db = 20.0 * (rms.max(1e-5)).log10();
        let is_low = current_db < -18.5;
        let target = if is_low {
            let boost_db = (-14.0 - current_db).min(9.5);
            10.0f32.powf(boost_db / 20.0).clamp(1.0, 3.0)
        } else {
            1.0
        };

        self.smoothed_gain = self.smoothed_gain * 0.94 + target * 0.06;
        if !is_low && self.smoothed_gain <= 1.01 {
            self.smoothed_gain = 1.0;
        }
        (is_low, self.smoothed_gain)
    }

    pub fn analyze_frequencies_pcm16(
        &mut self,
        samples: &[i16],
        channels: usize,
        out_magnitudes: &mut [f32; NUM_BANDS],
    ) {
        let ch = channels.clamp(1, 2);
        let frames = samples.len() / ch;
        if frames < 32 {
            *out_magnitudes = self.raw_magnitudes;
            return;
        }

        let window_size = frames.min(1024);
        let mut mono = [0.0f32; 1024];
        let mut mean = 0.0f32;
        for i in 0..window_size {
            let mut sum = 0.0f32;
            for c in 0..ch {
                sum += samples[i * ch + c] as f32 / 32768.0;
            }
            let m = sum / ch as f32;
            mono[i] = m;
            mean += m;
        }
        mean /= window_size as f32;
        for s in mono[..window_size].iter_mut() {
            *s -= mean;
        }

        self.analyze_mono_window(&mono[..window_size], out_magnitudes);
    }

    pub fn analyze_frequencies_f32(
        &mut self,
        samples: &[f32],
        channels: usize,
        out_magnitudes: &mut [f32; NUM_BANDS],
    ) {
        let ch = channels.clamp(1, 2);
        let frames = samples.len() / ch;
        if frames < 32 {
            *out_magnitudes = self.raw_magnitudes;
            return;
        }

        let window_size = frames.min(1024);
        let mut mono = [0.0f32; 1024];
        let mut mean = 0.0f32;
        for i in 0..window_size {
            let mut sum = 0.0f32;
            for c in 0..ch {
                let v = samples[i * ch + c];
                if v.is_finite() {
                    sum += v;
                }
            }
            let m = sum / ch as f32;
            mono[i] = m;
            mean += m;
        }
        mean /= window_size as f32;
        for s in mono[..window_size].iter_mut() {
            *s -= mean;
        }

        self.analyze_mono_window(&mono[..window_size], out_magnitudes);
    }

    fn analyze_mono_window(
        &mut self,
        mono: &[f32],
        out_magnitudes: &mut [f32; NUM_BANDS],
    ) {
        let window_size = mono.len();
        let sr = self.sample_rate.max(8000.0);
        let step = if window_size >= 512 { 2usize } else { 1usize };
        let denom = (window_size.saturating_sub(1)).max(1) as f32;

        for b in 0..NUM_BANDS {
            let freq = BAND_FREQUENCIES[b];
            let omega = 2.0 * PI * freq / sr;
            let mut real_sum = 0.0f32;
            let mut imag_sum = 0.0f32;
            let mut win_sum = 0.0f32;

            let mut n = 0;
            while n < window_size {
                let w = 0.5 * (1.0 - (2.0 * PI * (n as f32) / denom).cos());
                let s = mono[n] * w;
                let angle = omega * n as f32;
                real_sum += s * angle.cos();
                imag_sum += s * angle.sin();
                win_sum += w;
                n += step;
            }

            let dft_mag = (real_sum * real_sum + imag_sum * imag_sum).sqrt();
            let norm = win_sum.max(1.0);
            let raw_amp = (dft_mag / norm) * BAND_WEIGHTS[b];
            let boosted = (raw_amp * 15.5).max(0.0);
            let compressed = boosted.powf(0.54);
            let target = compressed.clamp(0.06, 0.98);
            let prev = self.raw_magnitudes[b];

            let smoothed = if target > prev {
                prev * 0.28 + target * 0.72
            } else {
                prev * 0.76 + target * 0.24
            };
            self.raw_magnitudes[b] = smoothed;
            out_magnitudes[b] = smoothed;
        }
    }
}


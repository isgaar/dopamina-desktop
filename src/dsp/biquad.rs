// Location: rust/src/dsp/biquad.rs
// Function: Double-precision (f64) Direct Form II Transposed Biquad Filter
// with bit-transparent zero-gain bypass and denormal flushing for Hi-Fi DSP.
// All Rights Reserved isgaar

use std::f64::consts::PI;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum FilterType {
    PeakingEq,
    LowShelf,
    HighShelf,
    BandPass,
}

#[derive(Clone, Debug)]
pub struct BiquadFilter {
    filter_type: FilterType,
    pub frequency: f32,
    pub q: f32,
    pub gain_db: f32,
    sample_rate: f32,
    pub is_bypass: bool,

    // Double-precision filter coefficients to eliminate low-freq pole drift
    b0: f64,
    b1: f64,
    b2: f64,
    a1: f64,
    a2: f64,

    // Double-precision state variables (DF-II Transposed)
    d1_l: f64,
    d2_l: f64,
    d1_r: f64,
    d2_r: f64,
}

impl BiquadFilter {
    pub fn new(filter_type: FilterType, frequency: f32, q: f32) -> Self {
        let mut filter = Self {
            filter_type,
            frequency,
            q: if q <= 0.01 { 0.70710678 } else { q },
            gain_db: 0.0,
            sample_rate: 44100.0,
            is_bypass: filter_type != FilterType::BandPass,
            b0: 1.0,
            b1: 0.0,
            b2: 0.0,
            a1: 0.0,
            a2: 0.0,
            d1_l: 0.0,
            d2_l: 0.0,
            d1_r: 0.0,
            d2_r: 0.0,
        };
        filter.recalculate();
        filter
    }

    pub fn is_bypassed(&self) -> bool {
        self.is_bypass
    }

    pub fn set_sample_rate(&mut self, rate: f32) {
        let valid_rate = rate.max(8000.0);
        if (self.sample_rate - valid_rate).abs() > 0.1 {
            self.sample_rate = valid_rate;
            self.recalculate();
        }
    }

    pub fn set_gain_db(&mut self, gain_db: f32) {
        let clamped = gain_db.clamp(-12.0, 12.0);
        if (self.gain_db - clamped).abs() > 1e-4 {
            self.gain_db = clamped;
            self.recalculate();
        }
    }

    pub fn set_frequency(&mut self, freq: f32) {
        let nyquist_guard = (self.sample_rate * 0.495).max(20000.0);
        let clamped = freq.clamp(20.0, nyquist_guard);
        if (self.frequency - clamped).abs() > 0.1 {
            self.frequency = clamped;
            self.recalculate();
        }
    }

    pub fn reset(&mut self) {
        self.d1_l = 0.0;
        self.d2_l = 0.0;
        self.d1_r = 0.0;
        self.d2_r = 0.0;
    }

    pub fn recalculate(&mut self) {
        let sr = self.sample_rate as f64;
        let f0 = self.frequency as f64;
        let g_db = self.gain_db as f64;

        if self.filter_type != FilterType::BandPass
            && (g_db.abs() < 1e-4 || f0 >= sr * 0.495)
        {
            self.is_bypass = true;
            self.b0 = 1.0;
            self.b1 = 0.0;
            self.b2 = 0.0;
            self.a1 = 0.0;
            self.a2 = 0.0;
            self.reset();
            return;
        }

        self.is_bypass = false;
        let omega = 2.0 * PI * (f0 / sr);
        let cos_omega = omega.cos();
        let sin_omega = omega.sin();
        let alpha = sin_omega / (2.0 * (self.q as f64));
        let a = 10.0_f64.powf(g_db / 40.0);

        let (b0, b1, b2, a0, a1, a2) = match self.filter_type {
            FilterType::PeakingEq => {
                let b0 = 1.0 + alpha * a;
                let b1 = -2.0 * cos_omega;
                let b2 = 1.0 - alpha * a;
                let a0 = 1.0 + alpha / a;
                let a1 = -2.0 * cos_omega;
                let a2 = 1.0 - alpha / a;
                (b0, b1, b2, a0, a1, a2)
            }
            FilterType::LowShelf => {
                let two_sqrt_a_alpha = 2.0 * a.sqrt() * alpha;
                let b0 = a * ((a + 1.0) - (a - 1.0) * cos_omega + two_sqrt_a_alpha);
                let b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * cos_omega);
                let b2 = a * ((a + 1.0) - (a - 1.0) * cos_omega - two_sqrt_a_alpha);
                let a0 = (a + 1.0) + (a - 1.0) * cos_omega + two_sqrt_a_alpha;
                let a1 = -2.0 * ((a - 1.0) + (a + 1.0) * cos_omega);
                let a2 = (a + 1.0) + (a - 1.0) * cos_omega - two_sqrt_a_alpha;
                (b0, b1, b2, a0, a1, a2)
            }
            FilterType::HighShelf => {
                let two_sqrt_a_alpha = 2.0 * a.sqrt() * alpha;
                let b0 = a * ((a + 1.0) + (a - 1.0) * cos_omega + two_sqrt_a_alpha);
                let b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cos_omega);
                let b2 = a * ((a + 1.0) + (a - 1.0) * cos_omega - two_sqrt_a_alpha);
                let a0 = (a + 1.0) - (a - 1.0) * cos_omega + two_sqrt_a_alpha;
                let a1 = 2.0 * ((a - 1.0) - (a + 1.0) * cos_omega);
                let a2 = (a + 1.0) - (a - 1.0) * cos_omega - two_sqrt_a_alpha;
                (b0, b1, b2, a0, a1, a2)
            }
            FilterType::BandPass => {
                let b0 = alpha;
                let b1 = 0.0;
                let b2 = -alpha;
                let a0 = 1.0 + alpha;
                let a1 = -2.0 * cos_omega;
                let a2 = 1.0 - alpha;
                (b0, b1, b2, a0, a1, a2)
            }
        };

        let inv_a0 = 1.0 / a0;
        self.b0 = b0 * inv_a0;
        self.b1 = b1 * inv_a0;
        self.b2 = b2 * inv_a0;
        self.a1 = a1 * inv_a0;
        self.a2 = a2 * inv_a0;
    }

    #[inline(always)]
    pub fn process_left(&mut self, input: f32) -> f32 {
        if self.is_bypass {
            return input;
        }
        let x = input as f64;
        let out = self.b0 * x + self.d1_l;
        if !out.is_finite() {
            self.reset();
            return input;
        }
        let mut d1 = self.b1 * x - self.a1 * out + self.d2_l;
        let mut d2 = self.b2 * x - self.a2 * out;
        if d1.abs() < 1e-24 {
            d1 = 0.0;
        }
        if d2.abs() < 1e-24 {
            d2 = 0.0;
        }
        self.d1_l = d1;
        self.d2_l = d2;
        out as f32
    }

    #[inline(always)]
    pub fn process_right(&mut self, input: f32) -> f32 {
        if self.is_bypass {
            return input;
        }
        let x = input as f64;
        let out = self.b0 * x + self.d1_r;
        if !out.is_finite() {
            self.reset();
            return input;
        }
        let mut d1 = self.b1 * x - self.a1 * out + self.d2_r;
        let mut d2 = self.b2 * x - self.a2 * out;
        if d1.abs() < 1e-24 {
            d1 = 0.0;
        }
        if d2.abs() < 1e-24 {
            d2 = 0.0;
        }
        self.d1_r = d1;
        self.d2_r = d2;
        out as f32
    }

    #[inline(always)]
    pub fn process_stereo(&mut self, left: f32, right: f32) -> (f32, f32) {
        if self.is_bypass {
            return (left, right);
        }
        (self.process_left(left), self.process_right(right))
    }
}

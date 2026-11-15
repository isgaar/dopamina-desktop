// Location: rust/src/dsp/equalizer.rs
// Function: Studio parametric equalizer (10/12-band), bass/mids/treble, soft-knee limiter & Haas 3D virtualizer.
// All Rights Reserved isgaar

use super::biquad::{BiquadFilter, FilterType};

pub const NUM_BANDS: usize = 10;
pub const BANDS_HZ: [f32; NUM_BANDS] = [
    31.25, 62.5, 125.0, 250.0, 500.0,
    1000.0, 2000.0, 4000.0, 8000.0, 16000.0,
];

pub const NUM_BANDS_12: usize = 12;
pub const BANDS_12_HZ: [f32; NUM_BANDS_12] = [
    30.0, 50.0, 90.0, 160.0, 300.0, 500.0,
    1000.0, 1600.0, 3000.0, 5000.0, 9000.0, 16000.0,
];

/// Studio C1-continuous soft-knee limiter.
/// Passband |x| <= 0.98 is 100% linear identity (zero harmonic distortion).
/// Above 0.98, smoothly saturates toward 1.0 with continuous first derivative.
#[inline(always)]
pub fn soft_clip(x: f32) -> f32 {
    const KNEE: f32 = 0.98;
    const HEADROOM: f32 = 1.0 - KNEE;
    let ax = x.abs();
    if ax <= KNEE {
        x
    } else {
        let excess = (ax - KNEE) / HEADROOM;
        let compressed = KNEE + HEADROOM * excess.tanh();
        compressed.copysign(x)
    }
}

/// Legacy unconditional tanh limiter used in the pre-fix implementation.
/// Kept only for comparative sound-loss benchmark tests.
#[inline(always)]
pub fn legacy_unconditional_tanh(x: f32) -> f32 {
    x.tanh()
}

pub struct ParametricEqualizer {
    pub is_enabled: bool,
    pub is_limiter_enabled: bool,
    filters: Vec<BiquadFilter>,
    bass_filter: BiquadFilter,
    mids_filter: BiquadFilter,
    treble_filter: BiquadFilter,

    pub bass_boost_percent: f32,
    pub mids_percent: f32,
    pub treble_percent: f32,
    pub preamp_gain_db: f32,
    pub virtualizer_percent: f32,
    pub compressor_percent: f32,
    pub balance: f32,

    preamp_linear: f32,
    headroom_linear: f32,
    comp_env: f32,

    // Haas delay line (24 samples delay for 3D stereo width)
    haas_delay_l: [f32; 24],
    haas_delay_r: [f32; 24],
    delay_idx: usize,
}

impl ParametricEqualizer {
    pub fn new() -> Self {
        Self::with_frequencies(&BANDS_HZ)
    }

    pub fn new_12_band() -> Self {
        Self::with_frequencies(&BANDS_12_HZ)
    }

    pub fn with_frequencies(freqs: &[f32]) -> Self {
        let mut filters = Vec::with_capacity(freqs.len());
        for &freq in freqs {
            filters.push(BiquadFilter::new(FilterType::PeakingEq, freq, 1.2));
        }

        let bass_filter = BiquadFilter::new(FilterType::LowShelf, 90.0, 0.75);
        let mids_filter = BiquadFilter::new(FilterType::PeakingEq, 1200.0, 0.9);
        let treble_filter = BiquadFilter::new(FilterType::HighShelf, 8500.0, 0.75);

        Self {
            is_enabled: true,
            is_limiter_enabled: true,
            filters,
            bass_filter,
            mids_filter,
            treble_filter,
            bass_boost_percent: 0.0,
            mids_percent: 0.0,
            treble_percent: 0.0,
            preamp_gain_db: 0.0,
            virtualizer_percent: 0.0,
            compressor_percent: 0.0,
            balance: 0.0,
            preamp_linear: 1.0,
            headroom_linear: 1.0,
            comp_env: 0.0,
            haas_delay_l: [0.0; 24],
            haas_delay_r: [0.0; 24],
            delay_idx: 0,
        }
    }

    pub fn set_sample_rate(&mut self, rate: f32) {
        for f in &mut self.filters {
            f.set_sample_rate(rate);
        }
        self.bass_filter.set_sample_rate(rate);
        self.mids_filter.set_sample_rate(rate);
        self.treble_filter.set_sample_rate(rate);
    }

    pub fn set_band_gain(&mut self, index: usize, gain_db: f32) {
        if index < self.filters.len() {
            self.filters[index].set_gain_db(gain_db);
            self.update_headroom();
        }
    }

    pub fn set_bass_boost(&mut self, percent: f32) {
        self.bass_boost_percent = percent.clamp(0.0, 100.0);
        let boost_db = (self.bass_boost_percent / 100.0) * 12.0;
        self.bass_filter.set_gain_db(boost_db);
        self.update_headroom();
    }

    pub fn set_mids(&mut self, percent: f32) {
        self.mids_percent = percent.clamp(0.0, 100.0);
        let boost_db = (self.mids_percent / 100.0) * 8.0;
        self.mids_filter.set_gain_db(boost_db);
        self.update_headroom();
    }

    pub fn set_treble(&mut self, percent: f32) {
        self.treble_percent = percent.clamp(0.0, 100.0);
        let boost_db = (self.treble_percent / 100.0) * 9.0;
        self.treble_filter.set_gain_db(boost_db);
        self.update_headroom();
    }

    pub fn set_preamp_gain(&mut self, gain_db: f32) {
        self.preamp_gain_db = gain_db.clamp(-12.0, 12.0);
        if self.preamp_gain_db.abs() < 0.001 {
            self.preamp_linear = 1.0;
        } else {
            self.preamp_linear = 10.0_f32.powf(self.preamp_gain_db / 20.0);
        }
    }

    pub fn set_virtualizer(&mut self, percent: f32) {
        self.virtualizer_percent = percent.clamp(0.0, 100.0);
    }

    pub fn set_compressor(&mut self, percent: f32) {
        self.compressor_percent = percent.clamp(0.0, 100.0);
    }

    pub fn set_balance(&mut self, bal: f32) {
        self.balance = bal.clamp(-1.0, 1.0);
    }

    fn update_headroom(&mut self) {
        let mut max_boost = (self.bass_boost_percent / 100.0) * 6.0;
        max_boost = max_boost.max((self.mids_percent / 100.0) * 4.0);
        max_boost = max_boost.max((self.treble_percent / 100.0) * 4.5);
        for f in &self.filters {
            if f.gain_db > max_boost {
                max_boost = f.gain_db;
            }
        }
        let atten_db = if max_boost > 2.0 {
            -(max_boost - 2.0) * 0.35
        } else {
            0.0
        };
        if atten_db.abs() < 0.001 {
            self.headroom_linear = 1.0;
        } else {
            self.headroom_linear = 10.0_f32.powf(atten_db / 20.0);
        }
    }

    #[inline(always)]
    pub fn is_flat(&self) -> bool {
        self.preamp_gain_db.abs() < 0.001
            && self.bass_filter.is_bypass
            && self.mids_filter.is_bypass
            && self.treble_filter.is_bypass
            && self.virtualizer_percent <= 0.001
            && self.compressor_percent <= 0.001
            && self.balance.abs() < 0.001
            && self.filters.iter().all(|f| f.is_bypass)
    }

    pub fn reset(&mut self) {
        for f in &mut self.filters {
            f.reset();
        }
        self.bass_filter.reset();
        self.mids_filter.reset();
        self.treble_filter.reset();
        self.comp_env = 0.0;
        self.haas_delay_l.fill(0.0);
        self.haas_delay_r.fill(0.0);
        self.delay_idx = 0;
    }

    #[inline(always)]
    pub fn process_stereo(&mut self, left: f32, right: f32) -> (f32, f32) {
        if !self.is_enabled {
            return (left, right);
        }

        // Bit-transparent fast path when EQ is flat and signal is within [-1.0, 1.0]
        if self.is_flat() {
            if !self.is_limiter_enabled || (left.abs() <= 1.0 && right.abs() <= 1.0) {
                return (left, right);
            }
            return (soft_clip(left), soft_clip(right));
        }

        let pre_scale = self.preamp_linear
            * if self.is_limiter_enabled {
                self.headroom_linear
            } else {
                1.0
            };

        let mut l = self.bass_filter.process_left(left * pre_scale);
        let mut r = self.bass_filter.process_right(right * pre_scale);

        l = self.mids_filter.process_left(l);
        r = self.mids_filter.process_right(r);

        l = self.treble_filter.process_left(l);
        r = self.treble_filter.process_right(r);

        for f in &mut self.filters {
            l = f.process_left(l);
            r = f.process_right(r);
        }

        // Dynamic range compressor (when enabled > 0%)
        if self.compressor_percent > 0.0 {
            let amt = self.compressor_percent / 100.0;
            let peak = l.abs().max(r.abs());
            let coeff = if peak > self.comp_env { 0.015 } else { 0.0008 };
            self.comp_env += coeff * (peak - self.comp_env);
            if self.comp_env > 0.35 {
                let gain_red = (0.35 / self.comp_env).powf(amt * 0.55);
                let makeup = 1.0 + amt * 0.25;
                l *= gain_red * makeup;
                r *= gain_red * makeup;
            }
        }

        // Haas 3D stereo virtualizer
        if self.virtualizer_percent > 0.0 {
            let amt = self.virtualizer_percent / 100.0;
            let width = 1.0 + amt * 1.35;
            let mid = (l + r) * 0.5;
            let side = (l - r) * 0.5;

            let d_diff =
                (self.haas_delay_l[self.delay_idx] - self.haas_delay_r[self.delay_idx]) * (amt * 0.22);
            self.haas_delay_l[self.delay_idx] = l;
            self.haas_delay_r[self.delay_idx] = r;
            self.delay_idx = (self.delay_idx + 1) % self.haas_delay_l.len();

            l = mid + side * width + d_diff;
            r = mid - side * width - d_diff;
        }

        // Stereo balance pan law
        if self.balance.abs() > 0.001 {
            let bal = self.balance;
            let l_gain = if bal > 0.0 { 1.0 - bal } else { 1.0 };
            let r_gain = if bal < 0.0 { 1.0 + bal } else { 1.0 };
            l *= l_gain;
            r *= r_gain;
        }

        // Studio C1 soft-knee limiter (linear up to 0.98)
        if self.is_limiter_enabled {
            l = soft_clip(l);
            r = soft_clip(r);
        }

        (l, r)
    }
}


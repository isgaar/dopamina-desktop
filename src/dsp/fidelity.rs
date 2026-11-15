// Location: rust/src/dsp/fidelity.rs
// Function: Real-time Goertzel spectral fidelity & fake-lossless transcode detector.
// All Rights Reserved isgaar

use std::f32::consts::PI;

pub const VERDICT_ANALYZING: i32 = 0;
pub const VERDICT_GENUINE_HI_RES: i32 = 1;
pub const VERDICT_GENUINE_LOSSLESS: i32 = 2;
pub const VERDICT_TRANSCODE_LOW: i32 = 3;
pub const VERDICT_LOSSY_ADPCM: i32 = 4;
pub const VERDICT_LOSSY_OK: i32 = 5;
pub const VERDICT_TRANSCODE_OK: i32 = 6;

const PROBE_FREQS: [f32; 6] = [
    1000.0,   // Ref base
    10000.0,  // Ref mid-high
    15800.0,  // MP3 128k shelf
    18500.0,  // YouTube / Opus 160k shelf
    20500.0,  // CD 44.1k boundary
    22500.0,  // Ultrasonic Hi-Res
];

pub struct FidelityDetector {
    sample_rate: f32,
    channel_count: usize,
    coefs: [f32; 6],
    s1: [f32; 6],
    s2: [f32; 6],
    energies: [f32; 6],
    accumulated_samples: usize,
    sample_window: usize,
    verdict: i32,
    cutoff_hz: i32,
    container_is_lossless: bool,
    container_is_hires: bool,
}

impl FidelityDetector {
    pub fn new() -> Self {
        let mut det = Self {
            sample_rate: 44100.0,
            channel_count: 2,
            coefs: [0.0; 6],
            s1: [0.0; 6],
            s2: [0.0; 6],
            energies: [0.0; 6],
            accumulated_samples: 0,
            sample_window: 16384,
            verdict: VERDICT_ANALYZING,
            cutoff_hz: 0,
            container_is_lossless: true,
            container_is_hires: false,
        };
        det.update_coefs();
        det
    }

    pub fn set_format(&mut self, sample_rate: f32, channel_count: usize) {
        self.sample_rate = sample_rate.max(8000.0);
        self.channel_count = channel_count.clamp(1, 2);
        self.update_coefs();
        self.reset();
    }

    pub fn set_container_meta(&mut self, is_lossless: bool, is_hires: bool, is_adpcm: bool) {
        self.container_is_lossless = is_lossless;
        self.container_is_hires = is_hires;
        if is_adpcm {
            self.verdict = VERDICT_LOSSY_ADPCM;
            self.cutoff_hz = (self.sample_rate * 0.45) as i32;
        } else if !is_lossless {
            self.verdict = VERDICT_LOSSY_OK;
            self.cutoff_hz = (self.sample_rate * 0.45) as i32;
        }
    }

    fn update_coefs(&mut self) {
        let sr = self.sample_rate;
        for i in 0..6 {
            let freq = PROBE_FREQS[i];
            if freq < sr * 0.5 {
                let w = 2.0 * PI * (freq / sr);
                self.coefs[i] = 2.0 * w.cos();
            } else {
                self.coefs[i] = 0.0;
            }
        }
    }

    pub fn reset(&mut self) {
        self.s1 = [0.0; 6];
        self.s2 = [0.0; 6];
        self.energies = [0.0; 6];
        self.accumulated_samples = 0;
        self.verdict = VERDICT_ANALYZING;
        self.cutoff_hz = 0;
    }

    pub fn process_samples_f32(&mut self, samples: &[f32]) {
        if self.verdict != VERDICT_ANALYZING {
            return;
        }

        let stride = self.channel_count;
        let mut idx = 0;
        let len = samples.len();

        while idx < len {
            let mono_sample = if stride == 2 && idx + 1 < len {
                (samples[idx] + samples[idx + 1]) * 0.5
            } else {
                samples[idx]
            };

            for b in 0..6 {
                let c = self.coefs[b];
                if c != 0.0 {
                    let s0 = mono_sample + c * self.s1[b] - self.s2[b];
                    self.s2[b] = self.s1[b];
                    self.s1[b] = s0;
                }
            }

            self.accumulated_samples += 1;
            idx += stride;

            if self.accumulated_samples >= self.sample_window {
                self.evaluate_verdict();
                break;
            }
        }
    }

    pub fn process_samples_i16(&mut self, samples: &[i16]) {
        if self.verdict != VERDICT_ANALYZING {
            return;
        }

        let stride = self.channel_count;
        let mut idx = 0;
        let len = samples.len();
        const INV_32768: f32 = 1.0 / 32768.0;

        while idx < len {
            let s_l = (samples[idx] as f32) * INV_32768;
            let mono_sample = if stride == 2 && idx + 1 < len {
                let s_r = (samples[idx + 1] as f32) * INV_32768;
                (s_l + s_r) * 0.5
            } else {
                s_l
            };

            for b in 0..6 {
                let c = self.coefs[b];
                if c != 0.0 {
                    let s0 = mono_sample + c * self.s1[b] - self.s2[b];
                    self.s2[b] = self.s1[b];
                    self.s1[b] = s0;
                }
            }

            self.accumulated_samples += 1;
            idx += stride;

            if self.accumulated_samples >= self.sample_window {
                self.evaluate_verdict();
                break;
            }
        }
    }

    fn evaluate_verdict(&mut self) {
        // Calculate power for each band
        for b in 0..6 {
            let c = self.coefs[b];
            let p = self.s1[b] * self.s1[b] + self.s2[b] * self.s2[b] - c * self.s1[b] * self.s2[b];
            self.energies[b] = p.max(1e-12);
        }

        let e_mid = self.energies[1]; // 10kHz
        let e_16k = self.energies[2]; // 15.8kHz
        let e_18k = self.energies[3]; // 18.5kHz
        let e_ultra = self.energies[5]; // 22.5kHz

        // If music has too low mid energy (quiet passage/intro), defer
        if e_mid < 0.0001 {
            self.s1 = [0.0; 6];
            self.s2 = [0.0; 6];
            self.accumulated_samples = 0;
            return;
        }

        let drop_16k = 10.0 * (e_16k / e_mid).log10();
        let drop_18k = 10.0 * (e_18k / e_mid).log10();
        let drop_ultra = if self.coefs[5] != 0.0 {
            10.0 * (e_ultra / e_mid).log10()
        } else {
            -100.0
        };

        if self.container_is_lossless {
            if drop_16k < -20.0 {
                // Hard 16kHz cliff (typical 128k MP3 transcode disguised as FLAC)
                self.verdict = VERDICT_TRANSCODE_LOW;
                self.cutoff_hz = 16000;
            } else if drop_18k < -22.0 {
                // 18-19kHz cutoff (YouTube Opus 160k transcode disguised as FLAC)
                self.verdict = VERDICT_TRANSCODE_LOW;
                self.cutoff_hz = 18500;
            } else if self.container_is_hires && drop_ultra > -26.0 {
                // True Hi-Res with ultrasonic energy
                self.verdict = VERDICT_GENUINE_HI_RES;
                self.cutoff_hz = (self.sample_rate * 0.48) as i32;
            } else {
                // Clean full range CD lossless
                self.verdict = VERDICT_GENUINE_LOSSLESS;
                self.cutoff_hz = 20500;
            }
        } else {
            if drop_16k < -20.0 {
                self.verdict = VERDICT_TRANSCODE_LOW;
                self.cutoff_hz = 16000;
            } else {
                self.verdict = VERDICT_LOSSY_OK;
                self.cutoff_hz = 19000;
            }
        }
    }

    pub fn get_verdict(&self) -> i32 {
        self.verdict
    }

    pub fn get_cutoff_hz(&self) -> i32 {
        self.cutoff_hz
    }
}

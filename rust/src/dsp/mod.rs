// Location: rust/src/dsp/mod.rs
// Function: Native Rust DSP engine and C FFI bridge for Dopamine Desktop audio
// processing (12-band 64-bit EQ, soft-knee limiter, dynamics, and crossfade).
// All Rights Reserved isgaar

pub mod biquad;
pub mod dynamics;
pub mod effects2;
pub mod equalizer;
pub mod fidelity;

use std::sync::Mutex;
use dynamics::{AudioDynamicsProcessor, CrossfadeEngine, NUM_BANDS};
use effects2::Effects2Engine;
use equalizer::ParametricEqualizer;
use fidelity::FidelityDetector;

pub struct DesktopDspEngine {
    pub eq: ParametricEqualizer,
    pub effects2: Effects2Engine,
    pub dynamics: AudioDynamicsProcessor,
    pub crossfade: CrossfadeEngine,
    pub fidelity: FidelityDetector,
    pub sample_rate: f32,
    pub channel_count: usize,
    pub use_stabilizer: bool,
    pub rem_fade_samples: i32,
    pub tot_fade_samples: i32,
}

impl DesktopDspEngine {
    pub fn new() -> Self {
        Self {
            eq: ParametricEqualizer::new_12_band(),
            effects2: Effects2Engine::new(),
            dynamics: AudioDynamicsProcessor::new(),
            crossfade: CrossfadeEngine::new(),
            fidelity: FidelityDetector::new(),
            sample_rate: 44100.0,
            channel_count: 2,
            use_stabilizer: false,
            rem_fade_samples: 0,
            tot_fade_samples: 0,
        }
    }

    pub fn set_format(&mut self, sample_rate: f32, channels: usize) {
        let sr = sample_rate.max(8000.0);
        let ch = channels.clamp(1, 2);
        self.sample_rate = sr;
        self.channel_count = ch;
        self.eq.set_sample_rate(sr);
        self.effects2.set_sample_rate(sr);
        self.dynamics.set_sample_rate(sr);
        self.crossfade.set_sample_rate(sr);
        self.fidelity.set_format(sr, ch);
    }

    pub fn reset(&mut self) {
        self.eq.reset();
        self.effects2.clear_buffers();
        self.dynamics.reset();
        self.rem_fade_samples = 0;
        self.tot_fade_samples = 0;
    }

    #[inline(always)]
    pub fn process_sample_pair(&mut self, left: f32, right: f32) -> (f32, f32) {
        let (eq_l, eq_r) = self.eq.process_stereo(left, right);
        self.effects2.process_stereo(eq_l, eq_r)
    }

    pub fn process_interleaved_f32(&mut self, samples: &mut [f32], channels: usize) {
        let ch = channels.clamp(1, 2);
        if ch != self.channel_count {
            self.channel_count = ch;
        }
        self.fidelity.process_samples_f32(samples);
        let (_, stab_gain) = self
            .dynamics
            .compute_stabilizer_gain_f32(samples, self.use_stabilizer);
        let out_env = self.crossfade.target_out_env;

        if ch == 2 {
            let pairs = samples.len() / 2;
            for i in 0..pairs {
                let in_l = if samples[i * 2].is_finite() { samples[i * 2] } else { 0.0 };
                let in_r = if samples[i * 2 + 1].is_finite() {
                    samples[i * 2 + 1]
                } else {
                    0.0
                };

                let (mut l, mut r) = self.crossfade.process_frame(
                    in_l,
                    in_r,
                    out_env,
                    &mut self.rem_fade_samples,
                    self.tot_fade_samples,
                    2,
                );

                if self.use_stabilizer {
                    l = (l * stab_gain).clamp(-1.0, 1.0);
                    r = (r * stab_gain).clamp(-1.0, 1.0);
                }

                let (l_out, r_out) = self.process_sample_pair(l, r);
                samples[i * 2] = l_out.clamp(-1.0, 1.0);
                samples[i * 2 + 1] = r_out.clamp(-1.0, 1.0);
            }
        } else {
            for s in samples.iter_mut() {
                let in_m = if s.is_finite() { *s } else { 0.0 };
                let (mut m, _) = self.crossfade.process_frame(
                    in_m,
                    in_m,
                    out_env,
                    &mut self.rem_fade_samples,
                    self.tot_fade_samples,
                    1,
                );
                if self.use_stabilizer {
                    m = (m * stab_gain).clamp(-1.0, 1.0);
                }
                let (m_out, _) = self.process_sample_pair(m, m);
                *s = m_out.clamp(-1.0, 1.0);
            }
        }
    }
}

static DESKTOP_DSP: Mutex<Option<DesktopDspEngine>> = Mutex::new(None);

fn with_dsp<F, R>(f: F) -> R
where
    F: FnOnce(&mut DesktopDspEngine) -> R,
{
    let mut guard = DESKTOP_DSP.lock().unwrap();
    if guard.is_none() {
        *guard = Some(DesktopDspEngine::new());
    }
    f(guard.as_mut().unwrap())
}

// =========================================================================
// C FFI Exports for C++ DspAudioBackend / EqualizerFilter / CrossfadeMixer
// =========================================================================

#[no_mangle]
pub extern "C" fn dopamine_dsp_init(sample_rate: f32, channels: i32) {
    with_dsp(|dsp| {
        dsp.set_format(sample_rate, channels.max(1) as usize);
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_reset() {
    with_dsp(|dsp| {
        dsp.reset();
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_set_eq_gains(
    gains_ptr: *const f32,
    count: i32,
    preamp_db: f32,
) {
    with_dsp(|dsp| {
        dsp.eq.set_preamp_gain(preamp_db);
        if !gains_ptr.is_null() && count > 0 {
            let gains = unsafe { std::slice::from_raw_parts(gains_ptr, count as usize) };
            for (idx, &g) in gains.iter().enumerate() {
                dsp.eq.set_band_gain(idx, g);
            }
        }
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_set_effects1(
    bass: f32,
    mids: f32,
    treble: f32,
    virtualizer: f32,
    compressor: f32,
    balance: f32,
) {
    with_dsp(|dsp| {
        dsp.eq.set_bass_boost(bass);
        dsp.eq.set_mids(mids);
        dsp.eq.set_treble(treble);
        dsp.eq.set_virtualizer(virtualizer);
        dsp.eq.set_compressor(compressor);
        dsp.eq.set_balance(balance);
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_set_effects2(
    echo_on: bool,
    echo_gain: f32,
    autowah_on: bool,
    autowah_gain: f32,
    reverb_on: bool,
    reverb_gain: f32,
    flanger_on: bool,
) {
    with_dsp(|dsp| {
        dsp.effects2.set_echo(echo_on, echo_gain);
        dsp.effects2.set_autowah(autowah_on, autowah_gain);
        dsp.effects2.set_reverb(reverb_on, reverb_gain);
        dsp.effects2.set_flanger(flanger_on);
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_set_limiter_enabled(enabled: bool) {
    with_dsp(|dsp| {
        dsp.eq.is_limiter_enabled = enabled;
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_set_stabilizer_enabled(enabled: bool) {
    with_dsp(|dsp| {
        dsp.use_stabilizer = enabled;
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_is_flat() -> bool {
    with_dsp(|dsp| dsp.eq.is_flat())
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_trigger_crossfade(fade_frames: i32) {
    with_dsp(|dsp| {
        let frames = fade_frames.max(0) as usize;
        let samples = (frames * dsp.channel_count.max(1)) as i32;
        dsp.tot_fade_samples = samples.max(1);
        dsp.rem_fade_samples = samples;
        dsp.crossfade.trigger_crossfade(frames);
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_set_crossfade_out_target(target_gain: f32) {
    with_dsp(|dsp| {
        dsp.crossfade.set_out_target(target_gain);
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_process_f32(
    samples_ptr: *mut f32,
    frames: i32,
    channels: i32,
) {
    if samples_ptr.is_null() || frames <= 0 || channels <= 0 {
        return;
    }
    let ch = channels.clamp(1, 2) as usize;
    let total = (frames as usize) * ch;
    let samples = unsafe { std::slice::from_raw_parts_mut(samples_ptr, total) };
    with_dsp(|dsp| {
        dsp.process_interleaved_f32(samples, ch);
    });
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_mix_equal_power(
    outgoing_ptr: *const f32,
    incoming_ptr: *const f32,
    output_ptr: *mut f32,
    frames: i32,
    channels: i32,
    progress_start: f32,
    progress_end: f32,
) {
    if output_ptr.is_null() || frames <= 0 || channels <= 0 {
        return;
    }
    let ch = channels.clamp(1, 2) as usize;
    let total = (frames as usize) * ch;
    let out = unsafe { std::slice::from_raw_parts_mut(output_ptr, total) };
    let zero_buf;
    let a: &[f32] = if outgoing_ptr.is_null() {
        zero_buf = vec![0.0f32; total];
        &zero_buf
    } else {
        unsafe { std::slice::from_raw_parts(outgoing_ptr, total) }
    };
    let zero_buf_b;
    let b: &[f32] = if incoming_ptr.is_null() {
        zero_buf_b = vec![0.0f32; total];
        &zero_buf_b
    } else {
        unsafe { std::slice::from_raw_parts(incoming_ptr, total) }
    };
    CrossfadeEngine::mix_streams_equal_power(a, b, out, ch, progress_start, progress_end);
}

#[no_mangle]
pub extern "C" fn dopamine_dsp_analyze_frequencies(
    samples_ptr: *const f32,
    sample_count: i32,
    channels: i32,
    out_mags_ptr: *mut f32,
) {
    if samples_ptr.is_null() || out_mags_ptr.is_null() || sample_count <= 0 {
        return;
    }
    let samples = unsafe { std::slice::from_raw_parts(samples_ptr, sample_count as usize) };
    let out_mags = unsafe { &mut *(out_mags_ptr as *mut [f32; NUM_BANDS]) };
    with_dsp(|dsp| {
        dsp.dynamics
            .analyze_frequencies_f32(samples, channels.max(1) as usize, out_mags);
    });
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::dsp::equalizer::legacy_unconditional_tanh;
    use std::f64::consts::PI;

    #[test]
    fn test_desktop_12_band_rust_dsp_zero_loss_and_crossfade() {
        let mut engine = DesktopDspEngine::new();
        engine.set_format(48000.0, 2);
        assert!(engine.eq.is_flat());

        let frames = 2048;
        let mut buf = Vec::with_capacity(frames * 2);
        let mut orig = Vec::with_capacity(frames * 2);
        for i in 0..frames {
            let s = (0.95 * (2.0 * PI * 1000.0 * i as f64 / 48000.0).sin()) as f32;
            orig.push(s);
            orig.push(-s);
            buf.push(s);
            buf.push(-s);
        }

        engine.process_interleaved_f32(&mut buf, 2);
        for i in 0..orig.len() {
            assert!(
                (orig[i] - buf[i]).abs() < 1e-6,
                "Expected bit-transparent output on flat 12-band EQ at sample {}",
                i
            );
        }

        // Compare against legacy tanh peak loss at 1.0 (0 dBFS)
        let legacy_peak = legacy_unconditional_tanh(1.0);
        let legacy_loss_db = 20.0 * legacy_peak.log10();
        assert!(legacy_loss_db < -2.30); // -2.37 dB loss before fix
        let (after_peak, _) = engine.eq.process_stereo(1.0, 1.0);
        let after_loss_db = 20.0 * after_peak.log10();
        assert!(after_loss_db.abs() < 1e-5); // 0.00 dB loss after fix
    }
}

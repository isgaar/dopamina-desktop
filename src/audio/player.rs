// Location: src/audio/player.rs
// Function: Dedicated real-time audio playback worker integrating Symphonia
// decoding, DesktopDspEngine processing, equal-power crossfade, spectrum FFT,
// and PulseAudio/PipeWire output.
// All Rights Reserved isgaar

use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::mpsc::{self, Receiver, Sender};
use std::sync::{Arc, Mutex};

use crate::core::settings::EqualizerSettings;
use crate::data::model::Track;
use crate::dsp::dynamics::NUM_BANDS;
use crate::dsp::DesktopDspEngine;

use super::decoder::DecodedStream;
use super::output::AudioSink;

#[derive(Debug, Clone)]
pub enum PlayerCommand {
    LoadTrack {
        track: Track,
        token: Option<String>,
        start_sec: f64,
        auto_play: bool,
    },
    Play,
    Pause,
    Stop,
    Seek(f64),
    SetVolume(f32),
    UpdateEqualizer(EqualizerSettings),
    UpdateAudioOptions {
        stabilizer: bool,
        crossfade_enabled: bool,
        crossfade_sec: f32,
        skip_silence: bool,
        buffer_ms: u32,
    },
}

#[derive(Debug, Clone)]
pub struct SharedPlaybackTelemetry {
    pub is_playing: Arc<AtomicBool>,
    pub position_ms: Arc<AtomicU64>,
    pub duration_ms: Arc<AtomicU64>,
    pub spectrum: Arc<Mutex<[f32; NUM_BANDS]>>,
    pub error_message: Arc<Mutex<Option<String>>>,
    pub track_finished: Arc<AtomicBool>,
}

impl Default for SharedPlaybackTelemetry {
    fn default() -> Self {
        Self {
            is_playing: Arc::new(AtomicBool::new(false)),
            position_ms: Arc::new(AtomicU64::new(0)),
            duration_ms: Arc::new(AtomicU64::new(0)),
            spectrum: Arc::new(Mutex::new([0.0; NUM_BANDS])),
            error_message: Arc::new(Mutex::new(None)),
            track_finished: Arc::new(AtomicBool::new(false)),
        }
    }
}

pub struct AudioPlayer {
    tx: Sender<PlayerCommand>,
    pub telemetry: SharedPlaybackTelemetry,
    pub current_track: Option<Track>,
    pub volume: f32,
}

impl AudioPlayer {
    pub fn spawn(initial_volume: f32, initial_eq: EqualizerSettings) -> Self {
        let (tx, rx) = mpsc::channel();
        let telemetry = SharedPlaybackTelemetry::default();
        let worker_telemetry = telemetry.clone();

        std::thread::Builder::new()
            .name(String::from("dopamine-audio"))
            .spawn(move || {
                audio_worker_loop(rx, worker_telemetry, initial_volume, initial_eq);
            })
            .expect("failed to spawn audio worker thread");

        Self {
            tx,
            telemetry,
            current_track: None,
            volume: initial_volume.clamp(0.0, 1.0),
        }
    }

    pub fn load_track(
        &mut self,
        track: Track,
        token: Option<String>,
        start_sec: f64,
        auto_play: bool,
    ) {
        self.current_track = Some(track.clone());
        let _ = self.tx.send(PlayerCommand::LoadTrack {
            track,
            token,
            start_sec,
            auto_play,
        });
    }

    pub fn play(&self) {
        let _ = self.tx.send(PlayerCommand::Play);
    }

    pub fn pause(&self) {
        let _ = self.tx.send(PlayerCommand::Pause);
    }

    pub fn toggle_play(&self) {
        if self.is_playing() {
            self.pause();
        } else {
            self.play();
        }
    }

    pub fn stop(&self) {
        let _ = self.tx.send(PlayerCommand::Stop);
    }

    pub fn seek(&self, seconds: f64) {
        let _ = self.tx.send(PlayerCommand::Seek(seconds.max(0.0)));
    }

    pub fn set_volume(&mut self, volume: f32) {
        self.volume = volume.clamp(0.0, 1.0);
        let _ = self.tx.send(PlayerCommand::SetVolume(self.volume));
    }

    pub fn update_equalizer(&self, eq: EqualizerSettings) {
        let _ = self.tx.send(PlayerCommand::UpdateEqualizer(eq));
    }

    pub fn update_audio_options(
        &self,
        stabilizer: bool,
        crossfade_enabled: bool,
        crossfade_sec: f32,
        skip_silence: bool,
        buffer_ms: u32,
    ) {
        let _ = self.tx.send(PlayerCommand::UpdateAudioOptions {
            stabilizer,
            crossfade_enabled,
            crossfade_sec,
            skip_silence,
            buffer_ms,
        });
    }

    pub fn is_playing(&self) -> bool {
        self.telemetry.is_playing.load(Ordering::Relaxed)
    }

    pub fn position_sec(&self) -> f64 {
        self.telemetry.position_ms.load(Ordering::Relaxed) as f64 / 1000.0
    }

    pub fn duration_sec(&self) -> f64 {
        self.telemetry.duration_ms.load(Ordering::Relaxed) as f64 / 1000.0
    }

    pub fn spectrum_snapshot(&self) -> [f32; NUM_BANDS] {
        *self.telemetry.spectrum.lock().unwrap()
    }

    pub fn take_finished(&self) -> bool {
        self.telemetry.track_finished.swap(false, Ordering::AcqRel)
    }

    pub fn last_error(&self) -> Option<String> {
        self.telemetry.error_message.lock().unwrap().clone()
    }
}

fn apply_eq_to_engine(dsp: &mut DesktopDspEngine, eq: &EqualizerSettings) {
    dsp.eq.set_preamp_gain(eq.preamp_db);
    for (idx, &gain) in eq.bands_db.iter().enumerate() {
        dsp.eq.set_band_gain(idx, gain);
    }
    dsp.eq.is_limiter_enabled = eq.limiter_enabled;
    dsp.eq.set_bass_boost(eq.z_bass);
    dsp.eq.set_mids(eq.z_mids);
    dsp.eq.set_treble(eq.z_treble);
    dsp.eq.set_compressor(eq.compressor);
    dsp.eq.set_balance(eq.balance);
    dsp.effects2.set_echo(eq.echo_enabled, eq.echo_gain);
    dsp.effects2.set_autowah(eq.autowah_enabled, eq.autowah_gain);
    dsp.effects2.set_reverb(eq.reverb_enabled, eq.reverb_gain);
    dsp.effects2.set_flanger(eq.flanger_enabled);
}

fn audio_worker_loop(
    rx: Receiver<PlayerCommand>,
    telemetry: SharedPlaybackTelemetry,
    initial_volume: f32,
    initial_eq: EqualizerSettings,
) {
    let mut dsp = DesktopDspEngine::new();
    apply_eq_to_engine(&mut dsp, &initial_eq);

    let mut buffer_ms = 1000u32;
    let mut sink = AudioSink::open(44_100, 2, buffer_ms);
    let mut stream: Option<DecodedStream> = None;
    let mut playing = false;
    let mut volume = initial_volume.clamp(0.0, 1.0);
    let mut frames_played: u64 = 0;
    let mut current_sr: u32 = 44_100;
    let mut current_ch: usize = 2;
    let mut crossfade_enabled = false;
    let mut crossfade_sec = 4.0f32;
    let mut skip_silence = false;
    let mut pcm = Vec::<f32>::with_capacity(8192);
    let mut bands = [0.0f32; NUM_BANDS];

    loop {
        let cmd = if playing && stream.is_some() {
            rx.try_recv().ok()
        } else {
            match rx.recv() {
                Ok(c) => Some(c),
                Err(_) => break,
            }
        };

        if let Some(cmd) = cmd {
            match cmd {
                PlayerCommand::LoadTrack {
                    track,
                    token,
                    start_sec,
                    auto_play,
                } => {
                    *telemetry.error_message.lock().unwrap() = None;
                    telemetry.track_finished.store(false, Ordering::Release);
                    sink.flush();

                    let source = if !track.path.trim().is_empty() {
                        track.path.as_str()
                    } else {
                        track.stream_url.as_str()
                    };

                    match DecodedStream::open(source, token.as_deref(), &track.format) {
                        Ok(mut decoded) => {
                            let sr = decoded.sample_rate.max(8_000);
                            let ch = decoded.channels.clamp(1, 2);
                            if sr != current_sr || ch != current_ch {
                                sink.reopen(sr, ch as u8, buffer_ms);
                                current_sr = sr;
                                current_ch = ch;
                            }
                            dsp.reset();
                            dsp.set_format(sr as f32, ch);
                            if crossfade_enabled && crossfade_sec > 0.1 {
                                let fade_frames = (crossfade_sec * sr as f32) as usize;
                                dsp.tot_fade_samples = (fade_frames * ch) as i32;
                                dsp.rem_fade_samples = (fade_frames * ch) as i32;
                                dsp.crossfade.trigger_crossfade(fade_frames);
                            }

                            if start_sec > 0.25 {
                                let _ = decoded.seek_to_sec(start_sec);
                            }
                            let dur = if decoded.duration_sec > 0.0 {
                                decoded.duration_sec
                            } else {
                                track.duration.max(0.0)
                            };
                            frames_played = (start_sec.max(0.0) * sr as f64) as u64;
                            telemetry
                                .duration_ms
                                .store((dur * 1000.0) as u64, Ordering::Relaxed);
                            telemetry
                                .position_ms
                                .store((start_sec.max(0.0) * 1000.0) as u64, Ordering::Relaxed);
                            stream = Some(decoded);
                            playing = auto_play;
                            telemetry.is_playing.store(playing, Ordering::Relaxed);
                        }
                        Err(err) => {
                            *telemetry.error_message.lock().unwrap() =
                                Some(format!("ERR_AUD_DECODE: {err}"));
                            stream = None;
                            playing = false;
                            telemetry.is_playing.store(false, Ordering::Relaxed);
                        }
                    }
                }
                PlayerCommand::Play => {
                    if stream.is_some() {
                        playing = true;
                        telemetry.is_playing.store(true, Ordering::Relaxed);
                    }
                }
                PlayerCommand::Pause => {
                    playing = false;
                    telemetry.is_playing.store(false, Ordering::Relaxed);
                    sink.flush();
                    *telemetry.spectrum.lock().unwrap() = [0.0; NUM_BANDS];
                }
                PlayerCommand::Stop => {
                    playing = false;
                    stream = None;
                    frames_played = 0;
                    telemetry.is_playing.store(false, Ordering::Relaxed);
                    telemetry.position_ms.store(0, Ordering::Relaxed);
                    sink.flush();
                    *telemetry.spectrum.lock().unwrap() = [0.0; NUM_BANDS];
                }
                PlayerCommand::Seek(sec) => {
                    if let Some(s) = stream.as_mut() {
                        sink.flush();
                        if s.seek_to_sec(sec).is_ok() {
                            frames_played = (sec * current_sr as f64) as u64;
                            telemetry
                                .position_ms
                                .store((sec * 1000.0) as u64, Ordering::Relaxed);
                        }
                    }
                }
                PlayerCommand::SetVolume(vol) => {
                    volume = vol.clamp(0.0, 1.0);
                }
                PlayerCommand::UpdateEqualizer(eq) => {
                    apply_eq_to_engine(&mut dsp, &eq);
                }
                PlayerCommand::UpdateAudioOptions {
                    stabilizer,
                    crossfade_enabled: cf_en,
                    crossfade_sec: cf_sec,
                    skip_silence: skip,
                    buffer_ms: buf_ms,
                } => {
                    dsp.use_stabilizer = stabilizer;
                    crossfade_enabled = cf_en;
                    crossfade_sec = cf_sec;
                    skip_silence = skip;
                    if buf_ms != buffer_ms {
                        buffer_ms = buf_ms;
                        sink.reopen(current_sr, current_ch as u8, buffer_ms);
                    }
                }
            }
            continue;
        }

        if !playing {
            continue;
        }

        let Some(active_stream) = stream.as_mut() else {
            playing = false;
            telemetry.is_playing.store(false, Ordering::Relaxed);
            continue;
        };

        match active_stream.next_pcm_packet(&mut pcm) {
            Ok(true) => {
                if skip_silence && is_packet_silent(&pcm) {
                    let packet_frames = pcm.len() / current_ch.max(1);
                    frames_played += packet_frames as u64;
                    continue;
                }

                dsp.process_interleaved_f32(&mut pcm, current_ch);

                if (volume - 1.0).abs() > 1e-4 {
                    for s in &mut pcm {
                        *s *= volume;
                    }
                }

                dsp.dynamics
                    .analyze_frequencies_f32(&pcm, current_ch, &mut bands);
                {
                    let mut spec = telemetry.spectrum.lock().unwrap();
                    for i in 0..NUM_BANDS {
                        spec[i] = spec[i] * 0.45 + bands[i] * 0.55;
                    }
                }

                let packet_frames = (pcm.len() / current_ch.max(1)) as u64;
                sink.write_f32(&pcm);
                frames_played += packet_frames;
                let latency_ms = (sink.latency_sec() * 1000.0) as u64;
                let played_ms = (frames_played * 1000) / current_sr.max(1) as u64;
                telemetry
                    .position_ms
                    .store(played_ms.saturating_sub(latency_ms), Ordering::Relaxed);
            }
            Ok(false) => {
                sink.drain();
                playing = false;
                stream = None;
                telemetry.is_playing.store(false, Ordering::Relaxed);
                telemetry.track_finished.store(true, Ordering::Release);
                *telemetry.spectrum.lock().unwrap() = [0.0; NUM_BANDS];
            }
            Err(err) => {
                *telemetry.error_message.lock().unwrap() = Some(format!("ERR_AUD_DECODE: {err}"));
                playing = false;
                stream = None;
                telemetry.is_playing.store(false, Ordering::Relaxed);
                telemetry.track_finished.store(true, Ordering::Release);
            }
        }
    }
}

fn is_packet_silent(samples: &[f32]) -> bool {
    if samples.is_empty() {
        return true;
    }
    let mut sum_sq = 0.0f32;
    let step = (samples.len() / 64).max(1);
    let mut count = 0usize;
    for &s in samples.iter().step_by(step) {
        sum_sq += s * s;
        count += 1;
    }
    let rms = (sum_sq / count.max(1) as f32).sqrt();
    rms < 0.0018
}


// Procedural Audio Synthesizer & Web Audio Bridge
// Inspired by Tribal/Electronic beats (3BallMTY vibe) to drive the visualizer immediately without external assets

export class AudioEngine {
  constructor() {
    this.audioCtx = null;
    this.analyser = null;
    this.sourceNode = null;
    this.audioElement = null;
    this.isPlaying = false;
    this.mode = 'synth'; // 'synth' or 'file' or 'mic'
    
    this.freqData = new Uint8Array(1024);
    this.waveData = new Uint8Array(1024);
    
    this.synthInterval = null;
    this.beatStep = 0;
    this.currentTime = 154; // default 02:34
    this.duration = 240;
  }

  init() {
    if (!this.audioCtx) {
      const AudioContextClass = window.AudioContext || window.webkitAudioContext;
      this.audioCtx = new AudioContextClass();
      this.analyser = this.audioCtx.createAnalyser();
      this.analyser.fftSize = 2048; // 1024 frequency bins
      this.analyser.smoothingTimeConstant = 0.8;
      
      this.freqData = new Uint8Array(this.analyser.frequencyBinCount);
      this.waveData = new Uint8Array(this.analyser.fftSize);
    }
    if (this.audioCtx.state === 'suspended') {
      this.audioCtx.resume();
    }
  }

  play() {
    this.init();
    this.isPlaying = true;

    if (this.mode === 'synth') {
      this.startSynth();
    } else if (this.mode === 'file' && this.audioElement) {
      this.audioElement.play();
    }
  }

  pause() {
    this.isPlaying = false;
    if (this.synthInterval) {
      clearInterval(this.synthInterval);
      this.synthInterval = null;
    }
    if (this.audioElement) {
      this.audioElement.pause();
    }
  }

  togglePlay() {
    if (this.isPlaying) {
      this.pause();
    } else {
      this.play();
    }
    return this.isPlaying;
  }

  loadAudioFile(file, onLoaded) {
    this.init();
    this.pause();
    this.mode = 'file';

    if (this.audioElement) {
      this.audioElement.pause();
    }

    const url = URL.createObjectURL(file);
    this.audioElement = new Audio(url);
    this.audioElement.crossOrigin = 'anonymous';

    if (this.sourceNode) {
      this.sourceNode.disconnect();
    }
    this.sourceNode = this.audioCtx.createMediaElementSource(this.audioElement);
    this.sourceNode.connect(this.analyser);
    this.analyser.connect(this.audioCtx.destination);

    this.audioElement.onloadedmetadata = () => {
      this.duration = this.audioElement.duration;
      if (onLoaded) onLoaded(file.name, this.duration);
      this.play();
    };

    this.audioElement.ontimeupdate = () => {
      this.currentTime = this.audioElement.currentTime;
    };
  }

  startSynth() {
    if (this.synthInterval) clearInterval(this.synthInterval);
    
    // Connect master synth bus to analyser and speakers
    if (!this.synthMaster) {
      this.synthMaster = this.audioCtx.createGain();
      this.synthMaster.gain.value = 0.6;
      this.synthMaster.connect(this.analyser);
      this.analyser.connect(this.audioCtx.destination);
    }

    // 132 BPM Tribal House Beat (Tribal Guarachero tempo)
    const stepTime = (60 / 132) / 4 * 1000;
    this.synthInterval = setInterval(() => {
      if (!this.isPlaying) return;
      this.triggerStep(this.beatStep);
      this.beatStep = (this.beatStep + 1) % 16;
      this.currentTime = (this.currentTime + stepTime / 1000) % this.duration;
    }, stepTime);
  }

  triggerStep(step) {
    const ctx = this.audioCtx;
    const now = ctx.currentTime;

    // Kick Drum (on 0, 4, 8, 12 - Four on the floor)
    if (step % 4 === 0) {
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.frequency.setValueAtTime(140, now);
      osc.frequency.exponentialRampToValueAtTime(32, now + 0.12);
      gain.gain.setValueAtTime(1.0, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + 0.16);
      osc.connect(gain);
      gain.connect(this.synthMaster);
      osc.start(now);
      osc.stop(now + 0.18);
    }

    // Snare / Tribal Clap (on 4, 12 + syncopated 14)
    if (step === 4 || step === 12 || step === 14) {
      const noise = ctx.createBufferSource();
      const buffer = ctx.createBuffer(1, ctx.sampleRate * 0.1, ctx.sampleRate);
      const data = buffer.getChannelData(0);
      for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
      noise.buffer = buffer;

      const filter = ctx.createBiquadFilter();
      filter.type = 'highpass';
      filter.frequency.value = 800;

      const gain = ctx.createGain();
      gain.gain.setValueAtTime(0.5, now);
      gain.gain.exponentialRampToValueAtTime(0.01, now + 0.12);

      noise.connect(filter);
      filter.connect(gain);
      gain.connect(this.synthMaster);
      noise.start(now);
      noise.stop(now + 0.13);
    }

    // Tribal Percussion / Triplet Guarachero hi-hats & rimshots (steps 2, 5, 7, 10, 13)
    if ([2, 5, 7, 10, 13].includes(step)) {
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.type = 'triangle';
      osc.frequency.setValueAtTime(420, now);
      osc.frequency.exponentialRampToValueAtTime(180, now + 0.05);
      gain.gain.setValueAtTime(0.35, now);
      gain.gain.exponentialRampToValueAtTime(0.01, now + 0.06);
      osc.connect(gain);
      gain.connect(this.synthMaster);
      osc.start(now);
      osc.stop(now + 0.07);
    }

    // Synth Melody Lead (3BallMTY style catchy high pitch synth)
    const melodyScale = [440, 523.25, 587.33, 659.25, 783.99, 880]; // A minor pentatonic
    if (step % 2 === 0) {
      const noteIdx = (Math.floor(step / 2) + Math.floor(now * 2)) % melodyScale.length;
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(melodyScale[noteIdx], now);
      
      gain.gain.setValueAtTime(0.2, now);
      gain.gain.exponentialRampToValueAtTime(0.01, now + 0.15);
      
      osc.connect(gain);
      gain.connect(this.synthMaster);
      osc.start(now);
      osc.stop(now + 0.16);
    }
  }

  getAudioData() {
    if (this.analyser) {
      this.analyser.getByteFrequencyData(this.freqData);
      this.analyser.getByteTimeDomainData(this.waveData);
    }
    return {
      freqData: this.freqData,
      waveData: this.waveData,
      isPlaying: this.isPlaying,
      currentTime: this.currentTime,
      duration: this.duration
    };
  }

  setVolume(val) {
    if (this.audioElement) this.audioElement.volume = val;
    if (this.synthMaster) this.synthMaster.gain.value = val * 0.8;
  }
}

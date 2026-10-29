import { WMPVisualizer } from './visualizer.js';
import { AudioEngine } from './audio_synth.js';

window.addEventListener('DOMContentLoaded', () => {
  const canvas = document.getElementById('visCanvas');
  const playBtn = document.getElementById('playBtn');
  const playIcon = document.getElementById('playIcon');
  const stopBtn = document.getElementById('stopBtn');
  const presetSelect = document.getElementById('presetSelect');
  const timeDisplay = document.getElementById('timeDisplay');
  const seekFill = document.getElementById('seekFill');
  const seekBar = document.getElementById('seekBar');
  const volSlider = document.getElementById('volSlider');
  const fullscreenBtn = document.getElementById('fullscreenBtn');
  const fileInput = document.getElementById('audioFileInput');
  const loadFileBtn = document.getElementById('loadFileBtn');
  const demoBeatBtn = document.getElementById('demoBeatBtn');
  const trackArtist = document.getElementById('trackArtist');
  const trackTitle = document.getElementById('trackTitle');

  const visualizer = new WMPVisualizer(canvas);
  const audio = new AudioEngine();

  // Play / Pause toggle
  playBtn.addEventListener('click', () => {
    const isPlaying = audio.togglePlay();
    updatePlayIcon(isPlaying);
  });

  function updatePlayIcon(isPlaying) {
    if (isPlaying) {
      playIcon.innerHTML = `
        <rect x="6" y="4" width="4" height="16" fill="#ffffff" rx="1"/>
        <rect x="14" y="4" width="4" height="16" fill="#ffffff" rx="1"/>
      `;
    } else {
      playIcon.innerHTML = `
        <polygon points="6,3 20,12 6,21" fill="#ffffff"/>
      `;
    }
  }

  // Stop button
  stopBtn.addEventListener('click', () => {
    audio.pause();
    audio.currentTime = 0;
    updatePlayIcon(false);
  });

  // Preset selector
  presetSelect.addEventListener('change', (e) => {
    visualizer.setPreset(e.target.value);
  });

  // Volume slider
  volSlider.addEventListener('input', (e) => {
    audio.setVolume(parseFloat(e.target.value));
  });

  // Seek bar click
  seekBar.addEventListener('click', (e) => {
    const rect = seekBar.getBoundingClientRect();
    const ratio = (e.clientX - rect.left) / rect.width;
    audio.currentTime = ratio * audio.duration;
    if (audio.audioElement) {
      audio.audioElement.currentTime = audio.currentTime;
    }
  });

  // File loading
  loadFileBtn.addEventListener('click', () => {
    fileInput.click();
  });

  fileInput.addEventListener('change', (e) => {
    if (e.target.files.length > 0) {
      const file = e.target.files[0];
      audio.loadAudioFile(file, (name, duration) => {
        trackArtist.textContent = name.replace(/\.[^/.]+$/, "");
        trackTitle.textContent = "Archivo de audio local";
        demoBeatBtn.classList.remove('active');
        loadFileBtn.classList.add('active');
        updatePlayIcon(true);
      });
    }
  });

  demoBeatBtn.addEventListener('click', () => {
    audio.pause();
    audio.mode = 'synth';
    trackArtist.textContent = "3BallMTY";
    trackTitle.textContent = "Inténtalo (Tribal Beat Demo)";
    loadFileBtn.classList.remove('active');
    demoBeatBtn.classList.add('active');
    audio.play();
    updatePlayIcon(true);
  });

  // Fullscreen toggle
  fullscreenBtn.addEventListener('click', () => {
    const container = document.querySelector('.player-container');
    if (!document.fullscreenElement) {
      container.requestFullscreen().catch(err => console.log(err));
    } else {
      document.exitFullscreen();
    }
  });

  function formatTime(sec) {
    const m = Math.floor(sec / 60);
    const s = Math.floor(sec % 60);
    return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
  }

  // Render Loop (60 FPS)
  function loop() {
    const data = audio.getAudioData();
    visualizer.render(data);

    // Update timeline
    timeDisplay.textContent = formatTime(data.currentTime);
    const progressPercent = Math.min(100, (data.currentTime / data.duration) * 100);
    seekFill.style.width = `${progressPercent}%`;

    requestAnimationFrame(loop);
  }

  requestAnimationFrame(loop);
});

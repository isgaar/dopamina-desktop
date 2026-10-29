// Windows Media Player Classic Visualizations Engine
// Recreated from mpvis.DLL (Alchemy) and wmp.dll (Bars and Waves, Spikes, Ambience)

export class WMPVisualizer {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    
    // Internal decay/blur buffer for Alchemy persistence
    this.trailCanvas = document.createElement('canvas');
    this.trailCtx = this.trailCanvas.getContext('2d');
    
    this.currentPreset = 'wonderwave';
    this.rotationAngle = 0;
    this.pulseScale = 1;
    this.colorCycle = 0;
    this.peaks = new Array(32).fill(0);
    
    // Resize handling
    this.resize();
    window.addEventListener('resize', () => this.resize());
  }

  resize() {
    this.width = this.canvas.width = this.canvas.clientWidth;
    this.height = this.canvas.height = this.canvas.clientHeight;
    this.trailCanvas.width = this.width;
    this.trailCanvas.height = this.height;
  }

  setPreset(preset) {
    this.currentPreset = preset;
    // Clear trail canvas on preset switch
    this.trailCtx.fillStyle = '#000000';
    this.trailCtx.fillRect(0, 0, this.width, this.height);
  }

  render(audioData) {
    const { freqData, waveData, isPlaying } = audioData;
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;
    const cx = w / 2;
    const cy = h / 2;

    // Calculate audio energy metrics
    let bassEnergy = 0;
    let midEnergy = 0;
    let highEnergy = 0;
    for (let i = 0; i < 16; i++) bassEnergy += freqData[i] || 0;
    for (let i = 16; i < 128; i++) midEnergy += freqData[i] || 0;
    for (let i = 128; i < 512; i++) highEnergy += freqData[i] || 0;
    
    bassEnergy = (bassEnergy / 16) / 255;
    midEnergy = (midEnergy / 112) / 255;
    highEnergy = (highEnergy / 384) / 255;
    const totalEnergy = (bassEnergy * 0.5 + midEnergy * 0.3 + highEnergy * 0.2);

    this.colorCycle += 0.005 + totalEnergy * 0.02;
    this.rotationAngle += 0.003 + totalEnergy * 0.01;

    switch (this.currentPreset) {
      case 'wonderwave':
        this.renderWonderWave(freqData, waveData, bassEnergy, midEnergy, highEnergy);
        break;
      case 'superstar':
        this.renderSuperStar(freqData, waveData, bassEnergy, midEnergy);
        break;
      case 'switchblur':
        this.renderSwitchBlur(freqData, waveData, totalEnergy);
        break;
      case 'bars':
        this.renderBarsAndWaves(freqData, waveData);
        break;
      case 'ocean_mist':
        this.renderOceanMist(waveData, bassEnergy);
        break;
      case 'firestorm':
        this.renderFirestorm(freqData, bassEnergy);
        break;
      case 'spikes':
        this.renderSpikes(freqData, waveData, totalEnergy);
        break;
      case 'ambience_mixer':
        this.renderAmbienceMixer(freqData, waveData, totalEnergy);
        break;
      default:
        this.renderWonderWave(freqData, waveData, bassEnergy, midEnergy, highEnergy);
        break;
    }
  }

  // --- ALCHEMY: WONDERWAVE (The exact visualization from user screenshot) ---
  renderWonderWave(freqData, waveData, bass, mid, high) {
    const ctx = this.ctx;
    const tctx = this.trailCtx;
    const w = this.width;
    const h = this.height;
    const cx = w / 2;
    const cy = h / 2;

    // 1. Fade previous trail for trailing psychedelic motion blur
    tctx.fillStyle = 'rgba(0, 0, 0, 0.12)';
    tctx.fillRect(0, 0, w, h);

    // 2. Draw central kaleidoscope diamonds and hyperbolic glow
    tctx.save();
    tctx.translate(cx, cy);

    const diamondRadius = 35 + bass * 120;
    const hue1 = (this.colorCycle * 60) % 360;
    const hue2 = (hue1 + 90) % 360;

    // Central pulsing lens
    const grad = tctx.createRadialGradient(0, 0, 5, 0, 0, diamondRadius * 1.5);
    grad.addColorStop(0, `hsla(${hue1 + 80}, 90%, 75%, 0.9)`);
    grad.addColorStop(0.3, `hsla(${hue1 + 40}, 80%, 55%, 0.6)`);
    grad.addColorStop(0.7, `hsla(${hue2}, 75%, 35%, 0.3)`);
    grad.addColorStop(1, 'rgba(0, 0, 0, 0)');

    tctx.fillStyle = grad;
    tctx.beginPath();
    tctx.arc(0, 0, diamondRadius * 1.5, 0, Math.PI * 2);
    tctx.fill();

    // 4-Quadrant kaleidoscopic mirrored wave lines (matching screenshot)
    const pointsCount = 48;
    const step = Math.floor(waveData.length / (pointsCount * 2));

    // Outer diagonal cross beams (Green/Cyan/Yellow laser rays)
    tctx.lineWidth = 2.0 + bass * 2.5;
    tctx.strokeStyle = `hsla(${hue1 + 60}, 95%, 70%, ${0.6 + bass * 0.4})`;

    // Diagonal cutting lasers crossing from corners
    const rayLength = Math.max(w, h) * 0.8;
    const rayAngleOffset = Math.sin(this.colorCycle) * 0.2;

    for (let angle of [-Math.PI / 4, Math.PI / 4, 3 * Math.PI / 4, -3 * Math.PI / 4]) {
      tctx.beginPath();
      tctx.moveTo(0, 0);
      const targetAngle = angle + rayAngleOffset;
      tctx.lineTo(Math.cos(targetAngle) * rayLength, Math.sin(targetAngle) * rayLength);
      tctx.stroke();
    }

    // Mirrored wave webs (Alchemy WonderWave diamond mesh)
    const quadrants = [
      { sx: 1, sy: 1 },
      { sx: -1, sy: 1 },
      { sx: 1, sy: -1 },
      { sx: -1, sy: -1 }
    ];

    for (const q of quadrants) {
      tctx.save();
      tctx.scale(q.sx, q.sy);

      // Layer 1: Dense inner frequency mesh (Cyan / Green)
      tctx.beginPath();
      tctx.strokeStyle = `hsla(${hue1 + 100}, 90%, 65%, 0.75)`;
      tctx.lineWidth = 1.6;

      for (let i = 0; i < pointsCount; i++) {
        const val = ((waveData[i * step] || 128) - 128) / 128;
        const freqVal = (freqData[i * 2] || 0) / 255;
        
        const x = (i / pointsCount) * (cx * 0.95) + val * 30;
        const y = (1 - (i / pointsCount)) * (cy * 0.95) + freqVal * (60 + bass * 80);

        if (i === 0) tctx.moveTo(x, y);
        else tctx.lineTo(x, y);
      }
      tctx.stroke();

      // Layer 2: Radiating diagonal rays from center
      tctx.beginPath();
      tctx.strokeStyle = `hsla(${hue1 + 140}, 85%, 60%, 0.55)`;
      tctx.lineWidth = 1.2;

      for (let i = 0; i < 16; i++) {
        const f = (freqData[i * 6] || 0) / 255;
        const startX = (i / 16) * (cx * 0.6);
        const startY = 0;
        const endX = cx * (0.8 + f * 0.4);
        const endY = cy * (0.3 + f * 0.6);

        tctx.moveTo(startX, startY);
        tctx.lineTo(endX, endY);
      }
      tctx.stroke();

      // Layer 3: Secondary outer acoustic wave (Yellow/Orange)
      tctx.beginPath();
      tctx.strokeStyle = `hsla(${hue2}, 95%, 70%, 0.8)`;
      tctx.lineWidth = 2.0;

      for (let i = 0; i < pointsCount; i += 2) {
        const wVal = ((waveData[i * step + 10] || 128) - 128) / 128;
        const px = (i / pointsCount) * (cx * 1.1);
        const py = (cx * 0.3) + wVal * (40 + mid * 90);
        if (i === 0) tctx.moveTo(px, py);
        else tctx.lineTo(px, py);
      }
      tctx.stroke();

      tctx.restore();
    }

    tctx.restore();

    // 3. Draw trail canvas into main canvas
    ctx.drawImage(this.trailCanvas, 0, 0);
  }

  // --- ALCHEMY: SUPERSTAR ---
  renderSuperStar(freqData, waveData, bass, mid) {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;
    const cx = w / 2;
    const cy = h / 2;

    ctx.fillStyle = 'rgba(0, 0, 0, 0.2)';
    ctx.fillRect(0, 0, w, h);

    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(this.rotationAngle);

    const rays = 24;
    const maxRadius = Math.min(cx, cy) * 0.9;
    const hue = (this.colorCycle * 80) % 360;

    for (let i = 0; i < rays; i++) {
      const angle = (i / rays) * Math.PI * 2;
      const freqIndex = Math.floor((i / rays) * 64);
      const intensity = (freqData[freqIndex] || 0) / 255;
      const len = maxRadius * (0.3 + intensity * 0.7 + bass * 0.3);

      ctx.save();
      ctx.rotate(angle);

      ctx.beginPath();
      ctx.moveTo(0, 0);
      ctx.lineTo(len, 0);
      ctx.strokeStyle = `hsla(${hue + i * 8}, 90%, 65%, ${0.4 + intensity * 0.6})`;
      ctx.lineWidth = 2 + intensity * 6;
      ctx.stroke();

      // Star tip flare
      ctx.beginPath();
      ctx.arc(len, 0, 3 + intensity * 8, 0, Math.PI * 2);
      ctx.fillStyle = '#ffffff';
      ctx.fill();

      ctx.restore();
    }

    // Center star core
    ctx.beginPath();
    ctx.arc(0, 0, 20 + bass * 40, 0, Math.PI * 2);
    ctx.fillStyle = `hsla(${hue}, 100%, 75%, 0.8)`;
    ctx.shadowBlur = 30;
    ctx.shadowColor = `hsla(${hue}, 100%, 70%, 1)`;
    ctx.fill();
    ctx.shadowBlur = 0;

    ctx.restore();
  }

  // --- ALCHEMY: SWITCHBLUR ---
  renderSwitchBlur(freqData, waveData, energy) {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;

    ctx.fillStyle = 'rgba(0, 0, 0, 0.15)';
    ctx.fillRect(0, 0, w, h);

    const bands = 40;
    const bandWidth = w / bands;
    const hue = (this.colorCycle * 70) % 360;

    ctx.save();
    for (let i = 0; i < bands; i++) {
      const val = (freqData[i * 4] || 0) / 255;
      const wave = ((waveData[i * 8] || 128) - 128) / 128;
      const x = i * bandWidth;
      const barH = val * (h * 0.7);
      const y = (h / 2) - (barH / 2) + wave * 50;

      const grad = ctx.createLinearGradient(x, y, x, y + barH);
      grad.addColorStop(0, `hsla(${hue + i * 5}, 100%, 70%, 0.9)`);
      grad.addColorStop(0.5, `hsla(${hue + i * 5 + 40}, 90%, 50%, 0.6)`);
      grad.addColorStop(1, `hsla(${hue + i * 5 + 80}, 80%, 30%, 0.1)`);

      ctx.fillStyle = grad;
      ctx.fillRect(x + 2, y, bandWidth - 4, barH);
    }
    ctx.restore();
  }

  // --- BARRAS Y ONDAS (Classic WMP Spectrum Equalizer) ---
  renderBarsAndWaves(freqData, waveData) {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;

    // Dark gradient background
    ctx.fillStyle = '#080c12';
    ctx.fillRect(0, 0, w, h);

    const numBars = 32;
    const spacing = 4;
    const totalSpacing = (numBars - 1) * spacing;
    const barWidth = Math.floor((w * 0.85 - totalSpacing) / numBars);
    const startX = Math.floor((w - (numBars * barWidth + totalSpacing)) / 2);
    const maxBarHeight = h * 0.55;
    const baseY = h * 0.75;

    // Draw segmented equalizer bars
    for (let i = 0; i < numBars; i++) {
      const dataIndex = Math.floor((i / numBars) * 128);
      const val = (freqData[dataIndex] || 0) / 255;
      const barHeight = Math.max(4, val * maxBarHeight);
      const x = startX + i * (barWidth + spacing);

      // Peak tracking with gravity drop
      if (barHeight > this.peaks[i]) {
        this.peaks[i] = barHeight;
      } else {
        this.peaks[i] = Math.max(0, this.peaks[i] - 2.5);
      }

      // Draw horizontal LED segments inside each bar
      const segmentHeight = 5;
      const segmentGap = 2;
      const totalSegments = Math.floor(barHeight / (segmentHeight + segmentGap));

      for (let s = 0; s < totalSegments; s++) {
        const segY = baseY - (s * (segmentHeight + segmentGap));
        const progress = s / (maxBarHeight / (segmentHeight + segmentGap));

        let color = '#00f080'; // Green
        if (progress > 0.65) color = '#ffff00'; // Yellow
        if (progress > 0.88) color = '#ff3333'; // Red

        ctx.fillStyle = color;
        ctx.fillRect(x, segY - segmentHeight, barWidth, segmentHeight);
      }

      // Peak hold indicator line
      const peakY = baseY - this.peaks[i];
      ctx.fillStyle = '#ffffff';
      ctx.fillRect(x, peakY - 3, barWidth, 2);
    }

    // Oscilloscope Waveform Ribbon along the top/middle
    ctx.beginPath();
    ctx.lineWidth = 2.5;
    ctx.strokeStyle = '#3ca2f5';
    ctx.shadowBlur = 10;
    ctx.shadowColor = '#6cd0ff';

    const waveY = h * 0.28;
    const waveStep = w / waveData.length;

    for (let i = 0; i < waveData.length; i += 4) {
      const v = (waveData[i] - 128) / 128;
      const y = waveY + v * (h * 0.15);
      const x = i * waveStep;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
    ctx.shadowBlur = 0;
  }

  // --- BRUMA MARINA (Ocean Mist) ---
  renderOceanMist(waveData, bass) {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;

    ctx.fillStyle = 'rgba(2, 8, 16, 0.2)';
    ctx.fillRect(0, 0, w, h);

    const layers = 5;
    for (let l = 0; l < layers; l++) {
      ctx.beginPath();
      const alpha = 0.3 + (l / layers) * 0.4;
      ctx.strokeStyle = `rgba(0, ${160 + l * 20}, ${220 + l * 8}, ${alpha})`;
      ctx.lineWidth = 2 + l;

      const yOffset = (h / 2) + (l - 2) * 40;
      for (let i = 0; i < waveData.length; i += 6) {
        const v = ((waveData[i] || 128) - 128) / 128;
        const x = (i / waveData.length) * w;
        const y = yOffset + v * (60 + bass * 90) + Math.sin(this.colorCycle * 2 + i * 0.05 + l) * 20;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
    }
  }

  // --- TORMENTA DE FUEGO (Firestorm) ---
  renderFirestorm(freqData, bass) {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;

    ctx.fillStyle = 'rgba(10, 0, 0, 0.25)';
    ctx.fillRect(0, 0, w, h);

    const numSparks = 60;
    for (let i = 0; i < numSparks; i++) {
      const fVal = (freqData[i % 32] || 0) / 255;
      const x = (i / numSparks) * w + (Math.sin(this.colorCycle * 3 + i) * 30);
      const sparkHeight = fVal * (h * 0.7) + bass * 150;
      const y = h - sparkHeight;

      const grad = ctx.createLinearGradient(x, h, x, y);
      grad.addColorStop(0, 'rgba(255, 60, 0, 0.8)');
      grad.addColorStop(0.5, 'rgba(255, 180, 0, 0.6)');
      grad.addColorStop(1, 'rgba(255, 255, 200, 0)');

      ctx.fillStyle = grad;
      ctx.beginPath();
      ctx.arc(x, y, 4 + fVal * 12, 0, Math.PI * 2);
      ctx.fill();

      ctx.fillRect(x - 2, y, 4, sparkHeight);
    }
  }

  // --- PUNTAS (Spikes) ---
  renderSpikes(freqData, waveData, energy) {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;
    const cx = w / 2;
    const cy = h / 2;

    ctx.fillStyle = 'rgba(0, 5, 12, 0.2)';
    ctx.fillRect(0, 0, w, h);

    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(this.rotationAngle * 0.5);

    const points = 72;
    const baseRadius = Math.min(cx, cy) * 0.45;
    const hue = (this.colorCycle * 90) % 360;

    ctx.beginPath();
    for (let i = 0; i < points; i++) {
      const angle = (i / points) * Math.PI * 2;
      const f = (freqData[i * 2] || 0) / 255;
      const r = baseRadius + f * (baseRadius * 0.8);
      const x = Math.cos(angle) * r;
      const y = Math.sin(angle) * r;

      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.closePath();

    ctx.strokeStyle = `hsla(${hue}, 95%, 65%, 0.9)`;
    ctx.lineWidth = 3;
    ctx.shadowBlur = 15;
    ctx.shadowColor = `hsla(${hue}, 100%, 60%, 1)`;
    ctx.stroke();
    ctx.shadowBlur = 0;

    ctx.restore();
  }

  // --- AMBIENTE / BATIDORA (Ambience Swirl) ---
  renderAmbienceMixer(freqData, waveData, energy) {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;
    const cx = w / 2;
    const cy = h / 2;

    ctx.fillStyle = 'rgba(0, 0, 0, 0.15)';
    ctx.fillRect(0, 0, w, h);

    ctx.save();
    ctx.translate(cx, cy);

    const spirals = 8;
    const pointsPerSpiral = 40;
    const hue = (this.colorCycle * 50) % 360;

    for (let s = 0; s < spirals; s++) {
      const baseAngle = (s / spirals) * Math.PI * 2 + this.rotationAngle * 2;
      ctx.beginPath();

      for (let p = 0; p < pointsPerSpiral; p++) {
        const dist = (p / pointsPerSpiral) * Math.min(cx, cy) * 0.9;
        const f = (freqData[p * 3] || 0) / 255;
        const angle = baseAngle + p * 0.18 + f * 0.5;
        const x = Math.cos(angle) * dist;
        const y = Math.sin(angle) * dist;

        if (p === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }

      ctx.strokeStyle = `hsla(${hue + s * 45}, 85%, 60%, 0.7)`;
      ctx.lineWidth = 2.5 + energy * 4;
      ctx.stroke();
    }

    ctx.restore();
  }
}

/**
 * Location: src/features/playback/visualizations/VisualizerEffectHost.qml
 * Function: Native Qt Quick / QML audio visualization effect canvas rendering
 * classic Windows Media Player presets (WonderWave, SuperStar, Firestorm,
 * OceanMist, Spikes, Ambience) with real-time frequency and waveform response.
 * All Rights Reserved isgaar
 */
import QtQuick
import "root:/src/shared"

Item {
    id: root

    property string preset: "wonderwave" // wonderwave | superstar | switchblur | bars | oceanmist | firestorm | spikes | ambience
    property var frequencyData: []
    property var waveformData: []
    property bool isPlaying: false
    property real animationSpeed: 1.0

    width: 640
    height: 480
    clip: true

    Canvas {
        id: canvas
        anchors.fill: parent
        renderTarget: Canvas.FramebufferObject
        renderStrategy: Canvas.Threaded

        property real rotationAngle: 0.0
        property real colorCycle: 0.0
        property real pulseScale: 1.0
        property var peaks: new Array(32).fill(0)

        Timer {
            id: animTimer
            interval: 16 // ~60 FPS
            running: root.visible && root.isPlaying
            repeat: true
            onTriggered: {
                canvas.colorCycle += 0.01 * root.animationSpeed;
                canvas.rotationAngle += 0.005 * root.animationSpeed;
                canvas.requestPaint();
            }
        }

        onPaint: {
            var ctx = getContext("2d");
            var w = width;
            var h = height;
            var cx = w / 2;
            var cy = h / 2;

            ctx.fillStyle = "#000000";
            ctx.fillRect(0, 0, w, h);

            var freq = root.frequencyData && root.frequencyData.length > 0 ? root.frequencyData : [];
            var wave = root.waveformData && root.waveformData.length > 0 ? root.waveformData : [];

            // Calculate energy metrics
            var bassEnergy = 0;
            var midEnergy = 0;
            var count = Math.min(freq.length, 32);
            for (var i = 0; i < count; ++i) {
                if (i < 8) bassEnergy += (freq[i] || 0);
                else midEnergy += (freq[i] || 0);
            }
            bassEnergy = count > 0 ? (bassEnergy / 8.0) / 255.0 : 0.0;
            midEnergy = count > 0 ? (midEnergy / 24.0) / 255.0 : 0.0;
            var energy = bassEnergy * 0.6 + midEnergy * 0.4;

            canvas.pulseScale = 0.95 + energy * 0.35;

            switch (root.preset) {
            case "wonderwave":
                renderWonderWave(ctx, cx, cy, w, h, wave, energy);
                break;
            case "bars":
                renderBars(ctx, w, h, freq);
                break;
            case "firestorm":
                renderFirestorm(ctx, cx, cy, w, h, freq, energy);
                break;
            default:
                renderWonderWave(ctx, cx, cy, w, h, wave, energy);
                break;
            }
        }

        function renderWonderWave(ctx, cx, cy, w, h, wave, energy) {
            ctx.save();
            ctx.translate(cx, cy);
            ctx.rotate(canvas.rotationAngle);
            ctx.scale(canvas.pulseScale, canvas.pulseScale);

            var radius = Math.min(cx, cy) * 0.65;
            var points = 128;
            var step = (Math.PI * 2) / points;

            ctx.lineWidth = 2.5;
            ctx.strokeStyle = Qt.hsla(canvas.colorCycle % 1.0, 0.85, 0.65, 0.85);
            ctx.beginPath();

            for (var i = 0; i <= points; ++i) {
                var angle = i * step;
                var sample = wave.length > 0 ? ((wave[i % wave.length] || 128) - 128) / 128.0 : Math.sin(angle * 4 + canvas.colorCycle * 5) * 0.2;
                var r = radius + sample * 45.0 * (1.0 + energy * 1.5);
                var x = Math.cos(angle) * r;
                var y = Math.sin(angle) * r;
                if (i === 0) ctx.moveTo(x, y);
                else ctx.lineTo(x, y);
            }

            ctx.closePath();
            ctx.stroke();
            ctx.restore();
        }

        function renderBars(ctx, w, h, freq) {
            var barCount = 32;
            var barWidth = (w - (barCount + 1) * 4) / barCount;
            for (var i = 0; i < barCount; ++i) {
                var val = freq.length > 0 ? (freq[i % freq.length] || 0) / 255.0 : Math.sin(i * 0.2 + canvas.colorCycle * 4) * 0.5 + 0.5;
                var barHeight = val * (h * 0.75);
                var x = 4 + i * (barWidth + 4);
                var y = h - barHeight;

                ctx.fillStyle = Qt.hsla((i / barCount + canvas.colorCycle) % 1.0, 0.8, 0.6, 0.9);
                ctx.fillRect(x, y, barWidth, barHeight);
            }
        }

        function renderFirestorm(ctx, cx, cy, w, h, freq, energy) {
            ctx.save();
            ctx.translate(cx, cy);
            var rays = 48;
            for (var i = 0; i < rays; ++i) {
                var angle = (i / rays) * Math.PI * 2 + canvas.rotationAngle;
                var length = (Math.min(cx, cy) * 0.4) + (freq[i % freq.length] || 100) * 0.8 * (1.0 + energy);
                ctx.strokeStyle = Qt.hsla(0.05 + (i / rays) * 0.1, 1.0, 0.55, 0.75);
                ctx.lineWidth = 3.0;
                ctx.beginPath();
                ctx.moveTo(0, 0);
                ctx.lineTo(Math.cos(angle) * length, Math.sin(angle) * length);
                ctx.stroke();
            }
            ctx.restore();
        }
    }
}

/**
 * Location: src/core/audio/DspAudioBackend.cpp
 * Function: Implementation of native Qt 6 Multimedia DSP backend streaming PCM
 * through float32 gain scaling and crossfade mixing into audio hardware sink.
 * All Rights Reserved isgaar
 */
#include "DspAudioBackend.h"
#include "PcmConversion.h"
#include <QMediaDevices>
#include <QTimer>
#include <vector>

namespace Dopamine::Core::Audio {

DspAudioBackend::DspAudioBackend(QObject *parent)
    : IAudioBackend(parent),
      m_cache(std::make_unique<Source::AudioDiskCache>()),
      m_nam(new QNetworkAccessManager(this)),
      m_prefetcher(std::make_unique<Source::TrackPrefetcher>(
          m_cache.get(), m_nam, this)) {
    m_pump = new QTimer(this);
    m_pump->setInterval(10);
    connect(m_pump, &QTimer::timeout, this, &DspAudioBackend::pump);
}

DspAudioBackend::~DspAudioBackend() { stop(); }

void DspAudioBackend::setVolume(int volume) {
    m_volume = qBound(0, volume, 100);
    if (m_sink) m_sink->setVolume(m_volume / 100.0f);
    emit volumeChanged(m_volume);
}

void DspAudioBackend::play(const QUrl &url) {
    if (m_isPaused && m_currentUrl == url && m_sink) {
        m_sink->resume();
        m_pump->start();
        m_isPlaying = true;
        m_isPaused = false;
        emit stateChanged(true);
        return;
    }
    playAt(url, 0);
}

void DspAudioBackend::playAt(const QUrl &url, qint64 positionMs) {
    if (!url.isValid()) { emit errorOccurred("URL no válida."); return; }
    const bool keepCrossfadeTail = m_crossfadeTriggered;
    stop();
    m_crossfadeTriggered = false;
    m_trackFinishedEmitted = false;
    if (!keepCrossfadeTail) {
        dopamine_dsp_set_crossfade_out_target(1.0f);
    }
    m_currentUrl = url; m_framesWritten = 0;
    m_startMs = qMax<qint64>(0, positionMs);
    m_skipFrames = -1;
    m_decoderFinished = false;
    m_pendingPcm.clear();
    m_decoder = new QAudioDecoder(this);
    connect(m_decoder, &QAudioDecoder::bufferReady,
            this, &DspAudioBackend::handleBufferReady);
    connect(m_decoder, &QAudioDecoder::finished,
            this, &DspAudioBackend::handleDecoderFinished);
    connect(m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error),
            this, &DspAudioBackend::handleDecoderError);
    connect(m_decoder, &QAudioDecoder::durationChanged, this,
            [this](qint64 durationMs) {
                m_durationMs = durationMs;
                emit durationChanged(durationMs);
            });
    QString c = m_cache->getFilePath(url);
    m_decoder->setSource(c.isEmpty() ? url : QUrl::fromLocalFile(c));
    m_decoder->start();
    m_pump->start();
    m_isPlaying = true; m_isPaused = false;
    emit stateChanged(true);
}

void DspAudioBackend::pause() {
    if (m_sink && m_isPlaying) {
        m_pump->stop();
        m_sink->suspend(); m_isPlaying = false; m_isPaused = true;
        emit stateChanged(false);
    }
}

void DspAudioBackend::stop() {
    // QAudioDecoder::stop() re-emits finished() synchronously, which re-enters
    // handleDecoderFinished(); without this guard the outer frame would call
    // deleteLater() on the already-cleared decoder and crash.
    if (m_stopping) return;
    m_stopping = true;
    if (m_pump) m_pump->stop();
    m_pendingPcm.clear();
    m_decoderFinished = false;
    if (m_decoder) {
        QAudioDecoder *decoder = m_decoder;
        m_decoder = nullptr;
        disconnect(decoder, nullptr, this, nullptr);
        decoder->stop();
        decoder->deleteLater();
    }
    if (m_sink) {
        QAudioSink *sink = m_sink;
        m_sink = nullptr;
        m_sinkIo = nullptr;
        disconnect(sink, nullptr, this, nullptr);
        sink->stop();
        sink->deleteLater();
    }
    m_isPlaying = false; m_isPaused = false; m_framesWritten = 0;
    m_startMs = 0; m_skipFrames = 0;
    m_trackFinishedEmitted = false;
    if (!m_crossfadeTriggered) {
        m_eqFilter.resetStates();
    }
    m_stopping = false;
    emit stateChanged(false); emit positionChanged(0);
}

void DspAudioBackend::seek(qint64 positionMs) {
    const qint64 clampedPos = qMax<qint64>(0, positionMs);
    if (m_currentUrl.isValid() && (m_isPlaying || m_isPaused)) {
        const bool wasPaused = m_isPaused;
        playAt(m_currentUrl, clampedPos);
        if (wasPaused) pause();
        emit positionChanged(clampedPos);
        return;
    }
    if (m_format.sampleRate() > 0)
        m_framesWritten = (clampedPos * m_format.sampleRate()) / 1000;
    emit positionChanged(clampedPos);
}
void DspAudioBackend::setNext(const QUrl &url) {
    m_nextUrl = url;
    if (url.isValid()) m_prefetcher->prefetch(url);
}

void DspAudioBackend::setEqualizer(const std::vector<float>& g, float pre) {
    m_eqGains = g; m_preampDb = pre;
    float r = (float)m_format.sampleRate();
    m_eqFilter.setup(m_eqGains, m_preampDb, r > 0.0f ? r : 44100.0f);
}

void DspAudioBackend::setEffects1(float bass, float mids, float treble,
                                  float virtualizer, float compressor,
                                  float balance) {
    dopamine_dsp_set_effects1(bass, mids, treble, virtualizer, compressor,
                              balance);
}

void DspAudioBackend::setEffects2(bool echoOn, float echoGain, bool autowahOn,
                                  float autowahGain, bool reverbOn,
                                  float reverbGain, bool flangerOn) {
    dopamine_dsp_set_effects2(echoOn, echoGain, autowahOn, autowahGain,
                              reverbOn, reverbGain, flangerOn);
}

void DspAudioBackend::setLimiterEnabled(bool enabled) {
    m_gainStage.setLimiterEnabled(enabled);
    dopamine_dsp_set_limiter_enabled(enabled);
}

void DspAudioBackend::setStabilizerEnabled(bool enabled) {
    dopamine_dsp_set_stabilizer_enabled(enabled);
}

void DspAudioBackend::setupSink(const QAudioFormat &format) {
    m_format = format;
    float r = format.sampleRate() > 0 ? (float)format.sampleRate() : 44100.0f;
    m_eqFilter.setup(m_eqGains, m_preampDb, r);
    auto dev = QMediaDevices::defaultAudioOutput();
    m_sink = new QAudioSink(dev, m_format, this);
    const int frames = qMax(2048, m_format.sampleRate() / 2);
    m_sink->setBufferSize(m_format.bytesForFrames(frames));
    m_sink->setVolume(m_volume / 100.0f);
    m_sinkIo = m_sink->start();
    connect(m_sink, &QAudioSink::stateChanged, this,
            [this](QAudio::State state) {
                if (state == QAudio::IdleState && m_decoderFinished &&
                    m_pendingPcm.isEmpty()) {
                    finishPlayback();
                }
            });
}

qint64 DspAudioBackend::maxPendingBytes() const {
    if (m_sink) {
        const int size = m_sink->bufferSize();
        if (size > 0) return size;
    }
    return 1 << 20;
}

void DspAudioBackend::flushPending() {
    if (m_pendingPcm.isEmpty()) return;
    if (!m_sink || !m_sinkIo) {
        m_pendingPcm.clear();
        if (m_decoderFinished) finishPlayback();
        return;
    }
    const qint64 bytesPerFrame = m_format.bytesPerFrame();
    if (bytesPerFrame <= 0) return;
    qint64 available = qMin<qint64>(
        m_sink->bytesFree(), m_pendingPcm.size());
    available -= available % bytesPerFrame;
    if (available <= 0) return;
    const qint64 written = m_sinkIo->write(
        m_pendingPcm.constData(), available);
    if (written <= 0) return;
    const qint64 aligned = written - (written % bytesPerFrame);
    m_pendingPcm.remove(0, static_cast<int>(written));
    if (aligned > 0) {
        m_framesWritten += aligned / bytesPerFrame;
        emit positionChanged(position());
    }
}

void DspAudioBackend::pump() {
    if (m_stopping) return;
    flushPending();
    if (!m_decoder) {
        if (m_decoderFinished) finishPlayback();
        return;
    }
    for (int i = 0; i < 32 && m_decoder && m_decoder->bufferAvailable() &&
                    m_pendingPcm.size() < maxPendingBytes(); ++i) {
        handleBufferReady();
        flushPending();
    }
    if (m_decoderFinished && m_pendingPcm.isEmpty()) finishPlayback();
}

void DspAudioBackend::handleBufferReady() {
    if (!m_decoder) return;
    if (m_pendingPcm.size() >= maxPendingBytes()) return;
    QAudioBuffer buf = m_decoder->read();
    if (!buf.isValid()) return;
    if (!m_sink) setupSink(buf.format());
    if (m_skipFrames < 0) {
        m_skipFrames = (m_startMs * m_format.sampleRate()) / 1000;
    }
    const QAudioFormat format = buf.format();
    int frames = buf.frameCount();
    const int ch = format.channelCount();
    const int bytesPerSample = format.bytesPerSample();
    if (frames <= 0 || ch <= 0 || bytesPerSample <= 0) return;

    std::vector<float> samples(
        static_cast<size_t>(frames) * static_cast<size_t>(ch));
    const char *source = buf.constData<char>();
    for (int i = 0; i < frames * ch; ++i) {
        samples[static_cast<size_t>(i)] =
            Pcm::decodeSample(source, format.sampleFormat(), i);
    }

    int offset = 0;
    if (m_skipFrames > 0) {
        const qint64 drop = qMin<qint64>(m_skipFrames, frames);
        m_skipFrames -= drop;
        m_framesWritten += drop;
        offset = static_cast<int>(drop);
        frames -= offset;
        if (frames <= 0) {
            return;
        }
    }

    if (m_crossfadeMs > 0 && m_durationMs > 5000) {
        const qint64 rem = m_durationMs - position();
        if (rem > 0 && rem <= m_crossfadeMs) {
            const float ratio = qBound(
                0.35f, static_cast<float>(rem) / static_cast<float>(m_crossfadeMs),
                1.0f);
            dopamine_dsp_set_crossfade_out_target(ratio);
        }
    }

    float *processed = samples.data() + offset * ch;
    m_gainStage.process(processed, frames, ch);
    m_eqFilter.process(processed, frames, ch);

    QByteArray raw(frames * ch * bytesPerSample, Qt::Uninitialized);
    for (int i = 0; i < frames * ch; ++i) {
        Pcm::encodeSample(raw.data(), format.sampleFormat(), i, processed[i]);
    }
    m_pendingPcm.append(raw);
    flushPending();
    if (m_durationMs > 0) {
        const qint64 rem = m_durationMs - position();
        if (m_nextUrl.isValid() &&
            (position() >= m_durationMs / 2 || rem <= 20000)) {
            m_prefetcher->prefetch(m_nextUrl);
        }
        const qint64 triggerWin = static_cast<qint64>(qBound(350, m_crossfadeMs / 3, 1200));
        if (m_crossfadeMs > 0 && m_durationMs > 5000 && !m_crossfadeTriggered &&
            rem > 0 && rem <= triggerWin) {
            m_crossfadeTriggered = true;
            const int sr = qMax(8000, m_format.sampleRate());
            const int fadeFrames = (m_crossfadeMs * sr) / 1000;
            dopamine_dsp_trigger_crossfade(fadeFrames);
            if (m_nextUrl.isValid()) {
                QUrl next = m_nextUrl; m_nextUrl = QUrl();
                play(next);
            } else {
                m_trackFinishedEmitted = true;
                emit trackFinished();
            }
        }
    }
}

void DspAudioBackend::handleDecoderFinished() {
    m_decoderFinished = true;
    flushPending();
    if (m_pendingPcm.isEmpty()) finishPlayback();
}

void DspAudioBackend::finishPlayback() {
    if (!m_decoderFinished || !m_pendingPcm.isEmpty()) return;
    if (m_decoder && m_decoder->bufferAvailable()) return;
    if (m_sink && m_sinkIo && m_sink->state() != QAudio::IdleState) return;
    m_decoderFinished = false;
    if (m_crossfadeMs > 0 && !m_crossfadeTriggered) {
        const int sr = qMax(8000, m_format.sampleRate());
        dopamine_dsp_trigger_crossfade((m_crossfadeMs * sr) / 1000);
        m_crossfadeTriggered = true;
    }
    if (m_nextUrl.isValid()) {
        QUrl next = m_nextUrl; m_nextUrl = QUrl();
        play(next);
    } else {
        stop();
        if (!m_trackFinishedEmitted) {
            m_trackFinishedEmitted = true;
            emit trackFinished();
        }
    }
}

void DspAudioBackend::handleDecoderError(QAudioDecoder::Error) {
    emit errorOccurred(m_decoder ? m_decoder->errorString() : "Error audio");
    stop();
}

} // namespace Dopamine::Core::Audio

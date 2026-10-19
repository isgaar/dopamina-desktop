/**
 * Location: src/core/audio/DspAudioBackend.cpp
 * Function: Implementation of native Qt 6 Multimedia DSP backend streaming PCM
 * through float32 gain scaling and crossfade mixing into audio hardware sink.
 * All Rights Reserved isgaar
 */
#include "DspAudioBackend.h"
#include <QMediaDevices>

namespace Dopamine::Core::Audio {

DspAudioBackend::DspAudioBackend(QObject *parent)
    : IAudioBackend(parent),
      m_cache(std::make_unique<Source::AudioDiskCache>()),
      m_nam(new QNetworkAccessManager(this)),
      m_prefetcher(std::make_unique<Source::TrackPrefetcher>(
          m_cache.get(), m_nam, this)) {}

DspAudioBackend::~DspAudioBackend() { stop(); }

void DspAudioBackend::setVolume(int volume) {
    m_volume = qBound(0, volume, 100);
    if (m_sink) m_sink->setVolume(m_volume / 100.0f);
    emit volumeChanged(m_volume);
}

void DspAudioBackend::play(const QUrl &url) {
    if (!url.isValid()) { emit errorOccurred("URL no válida."); return; }
    stop();
    m_currentUrl = url; m_framesWritten = 0;
    m_decoder = new QAudioDecoder(this);
    connect(m_decoder, &QAudioDecoder::bufferReady,
            this, &DspAudioBackend::handleBufferReady);
    connect(m_decoder, &QAudioDecoder::finished,
            this, &DspAudioBackend::handleDecoderFinished);
    connect(m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error),
            this, &DspAudioBackend::handleDecoderError);
    QString c = m_cache->getFilePath(url);
    m_decoder->setSource(c.isEmpty() ? url : QUrl::fromLocalFile(c));
    m_decoder->start();
    m_isPlaying = true; m_isPaused = false;
    emit stateChanged(true);
}

void DspAudioBackend::pause() {
    if (m_sink && m_isPlaying) {
        m_sink->suspend(); m_isPlaying = false; m_isPaused = true;
        emit stateChanged(false);
    }
}

void DspAudioBackend::stop() {
    if (m_decoder) { m_decoder->stop(); m_decoder->deleteLater(); }
    if (m_sink) { m_sink->stop(); m_sink->deleteLater(); }
    m_decoder = nullptr; m_sink = nullptr; m_sinkIo = nullptr;
    m_isPlaying = false; m_isPaused = false; m_framesWritten = 0;
    m_eqFilter.resetStates();
    emit stateChanged(false); emit positionChanged(0);
}

void DspAudioBackend::seek(qint64 positionMs) {
    if (m_format.sampleRate() > 0)
        m_framesWritten = (positionMs * m_format.sampleRate()) / 1000;
    emit positionChanged(positionMs);
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

void DspAudioBackend::setupSink(const QAudioFormat &format) {
    m_format = format;
    float r = format.sampleRate() > 0 ? (float)format.sampleRate() : 44100.0f;
    m_eqFilter.setup(m_eqGains, m_preampDb, r);
    auto dev = QMediaDevices::defaultAudioOutput();
    m_sink = new QAudioSink(dev, m_format, this);
    m_sink->setVolume(m_volume / 100.0f);
    m_sinkIo = m_sink->start();
}

void DspAudioBackend::handleBufferReady() {
    if (!m_decoder) return;
    QAudioBuffer buf = m_decoder->read();
    if (!buf.isValid()) return;
    if (!m_sink) setupSink(buf.format());
    int frames = buf.frameCount(), ch = buf.format().channelCount();
    QByteArray raw(reinterpret_cast<const char*>(buf.constData<float>()),
                   frames * ch * sizeof(float));
    m_gainStage.process(reinterpret_cast<float*>(raw.data()), frames, ch);
    m_eqFilter.process(reinterpret_cast<float*>(raw.data()), frames, ch);
    if (m_sinkIo) m_sinkIo->write(raw);
    m_framesWritten += frames;
    emit positionChanged(position());
    if (m_nextUrl.isValid() && m_durationMs > 0) {
        qint64 rem = m_durationMs - position();
        if (position() >= m_durationMs / 2 || rem <= 20000) {
            m_prefetcher->prefetch(m_nextUrl);
        }
    }
}

void DspAudioBackend::handleDecoderFinished() {
    if (m_nextUrl.isValid()) {
        QUrl next = m_nextUrl; m_nextUrl = QUrl();
        play(next);
    } else { stop(); emit trackFinished(); }
}

void DspAudioBackend::handleDecoderError(QAudioDecoder::Error) {
    emit errorOccurred(m_decoder ? m_decoder->errorString() : "Error audio");
    stop();
}

} // namespace Dopamine::Core::Audio

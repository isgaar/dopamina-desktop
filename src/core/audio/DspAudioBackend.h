/**
 * Location: src/core/audio/DspAudioBackend.h
 * Function: High fidelity Qt 6 Multimedia audio backend driving QAudioDecoder,
 * float32 DSP processing, equal-power crossfades, and hardware QAudioSink.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_DSPAUDIOBACKEND_H
#define DOPAMINE_CORE_AUDIO_DSPAUDIOBACKEND_H

#include "IAudioBackend.h"
#include "dsp/GainStage.h"
#include "automix/CrossfadeMixer.h"
#include "dsp/EqSpec.h"
#include "source/AudioDiskCache.h"
#include "source/TrackPrefetcher.h"
#include <QAudioSink>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QByteArray>
#include <memory>
#include <cmath>

class QTimer;

namespace Dopamine::Core::Audio {

class DspAudioBackend : public IAudioBackend {
    Q_OBJECT
public:
    explicit DspAudioBackend(QObject *parent = nullptr);
    ~DspAudioBackend() override;

    void play(const QUrl &url) override;
    void playAt(const QUrl &url, qint64 positionMs) override;
    void pause() override;
    void stop() override;
    void seek(qint64 positionMs) override;
    qint64 duration() const override { return m_durationMs; }
    void setVolume(int volume) override;
    int volume() const override { return m_volume; }
    bool isPlaying() const override { return m_isPlaying; }
    QUrl currentUrl() const override { return m_currentUrl; }
    qint64 position() const override {
        if (m_sink)
            return m_startMs + m_sink->processedUSecs() / 1000;
        if (m_format.sampleRate() > 0)
            return (m_framesWritten * 1000) / m_format.sampleRate();
        return 0;
    }

    void setNext(const QUrl &url) override;
    void setGainDb(float gainDb) override {
        m_gainStage.setGain(std::pow(10.0f, gainDb / 20.0f));
    }
    void setCrossfadeMs(int ms) override { m_crossfadeMs = ms; }
    void setBearerToken(const QByteArray &token) {
        m_token = token;
        if (m_prefetcher) m_prefetcher->setBearerToken(token);
    }
    void setEqualizer(const std::vector<float>& gains, float preampDb);
    void setEffects1(float bass, float mids, float treble, float virtualizer,
                     float compressor, float balance);
    void setEffects2(bool echoOn, float echoGain, bool autowahOn,
                     float autowahGain, bool reverbOn, float reverbGain,
                     bool flangerOn);
    void setLimiterEnabled(bool enabled);
    void setStabilizerEnabled(bool enabled);
    bool isFilterFlat() const { return m_eqFilter.isFlat(); }

private slots:
    void handleBufferReady();
    void handleDecoderFinished();
    void handleDecoderError(QAudioDecoder::Error error);
    void pump();

private:
    void setupSink(const QAudioFormat &format);
    void flushPending();
    void finishPlayback();
    qint64 maxPendingBytes() const;

    QUrl m_currentUrl;
    QUrl m_nextUrl;
    QByteArray m_token;
    QAudioDecoder *m_decoder = nullptr;
    QAudioDecoder *m_nextDecoder = nullptr;
    QAudioSink *m_sink = nullptr;
    QIODevice *m_sinkIo = nullptr;
    QAudioFormat m_format;

    Dsp::GainStage m_gainStage;
    Dsp::EqualizerFilter m_eqFilter;
    std::vector<float> m_eqGains;
    float m_preampDb = 0.0f;
    Dsp::CrossfadeMixer m_mixer;
    std::unique_ptr<Source::AudioDiskCache> m_cache;
    QNetworkAccessManager *m_nam = nullptr;
    std::unique_ptr<Source::TrackPrefetcher> m_prefetcher;
    QTimer *m_pump = nullptr;

    QByteArray m_pendingPcm;
    qint64 m_framesWritten = 0;
    qint64 m_startMs = 0;
    qint64 m_skipFrames = 0;
    qint64 m_durationMs = 0;
    int m_volume = 100;
    int m_crossfadeMs = 0;
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_stopping = false;
    bool m_decoderFinished = false;
    bool m_crossfadeTriggered = false;
};

} // namespace Dopamine::Core::Audio

#endif // DOPAMINE_CORE_AUDIO_DSPAUDIOBACKEND_H

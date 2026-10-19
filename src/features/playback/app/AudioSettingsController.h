/**
 * Location: src/features/playback/app/AudioSettingsController.h
 * Function: Audio configuration controller providing High-Res output mode,
 * 32-bit float studio codecs, gapless buffers, and crossfade duration to UI.
 * All Rights Reserved isgaar
 */
#pragma once

#include <QObject>
#include <QStringList>

namespace Dopamine::Features::Playback::App {

class AudioSettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool highResEnabled READ highResEnabled WRITE setHighResEnabled
               NOTIFY settingsChanged)
    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate
               NOTIFY settingsChanged)
    Q_PROPERTY(QString bitDepth READ bitDepth WRITE setBitDepth
               NOTIFY settingsChanged)
    Q_PROPERTY(int bufferSizeMs READ bufferSizeMs WRITE setBufferSizeMs
               NOTIFY settingsChanged)
    Q_PROPERTY(double crossfadeSec READ crossfadeSec WRITE setCrossfadeSec
               NOTIFY settingsChanged)
    Q_PROPERTY(bool gaplessEnabled READ gaplessEnabled
               WRITE setGaplessEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString replayGainMode READ replayGainMode
               WRITE setReplayGainMode NOTIFY settingsChanged)
    Q_PROPERTY(QString streamingFormat READ streamingFormat
               WRITE setStreamingFormat NOTIFY settingsChanged)
    Q_PROPERTY(QStringList availableSampleRates
               READ availableSampleRates CONSTANT)
    Q_PROPERTY(QStringList availableBitDepths
               READ availableBitDepths CONSTANT)
    Q_PROPERTY(QStringList availableFormats
               READ availableFormats CONSTANT)

public:
    explicit AudioSettingsController(QObject *parent = nullptr);

    bool highResEnabled() const { return m_highResEnabled; }
    void setHighResEnabled(bool en);

    int sampleRate() const { return m_sampleRate; }
    void setSampleRate(int rate);

    QString bitDepth() const { return m_bitDepth; }
    void setBitDepth(const QString &depth);

    int bufferSizeMs() const { return m_bufferSizeMs; }
    void setBufferSizeMs(int ms);

    double crossfadeSec() const { return m_crossfadeSec; }
    void setCrossfadeSec(double sec);

    bool gaplessEnabled() const { return m_gaplessEnabled; }
    void setGaplessEnabled(bool en);

    QString replayGainMode() const { return m_replayGainMode; }
    void setReplayGainMode(const QString &mode);

    QString streamingFormat() const { return m_streamingFormat; }
    void setStreamingFormat(const QString &fmt);

    QStringList availableSampleRates() const;
    QStringList availableBitDepths() const;
    QStringList availableFormats() const;

signals:
    void settingsChanged();

private:
    bool m_highResEnabled = true;
    int m_sampleRate = 48000;
    QString m_bitDepth = "32-bit Float";
    int m_bufferSizeMs = 250;
    double m_crossfadeSec = 6.0;
    bool m_gaplessEnabled = true;
    QString m_replayGainMode = "Track";
    QString m_streamingFormat = "Opus (192 kbps)";
};

} // namespace Dopamine::Features::Playback::App

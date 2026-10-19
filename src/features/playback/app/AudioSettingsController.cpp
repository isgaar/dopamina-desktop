/**
 * Location: src/features/playback/app/AudioSettingsController.cpp
 * Function: Implementation of AudioSettingsController handling High-Res audio
 * modes, 32-bit floating output formats, buffer limits, and crossfade timing.
 * All Rights Reserved isgaar
 */
#include "features/playback/app/AudioSettingsController.h"

namespace Dopamine::Features::Playback::App {

AudioSettingsController::AudioSettingsController(QObject *parent)
    : QObject(parent) {}

void AudioSettingsController::setHighResEnabled(bool en) {
    if (m_highResEnabled == en) return;
    m_highResEnabled = en;
    emit settingsChanged();
}

void AudioSettingsController::setSampleRate(int rate) {
    if (m_sampleRate == rate) return;
    m_sampleRate = rate;
    emit settingsChanged();
}

void AudioSettingsController::setBitDepth(const QString &depth) {
    if (m_bitDepth == depth) return;
    m_bitDepth = depth;
    emit settingsChanged();
}

void AudioSettingsController::setBufferSizeMs(int ms) {
    if (m_bufferSizeMs == ms) return;
    m_bufferSizeMs = ms;
    emit settingsChanged();
}

void AudioSettingsController::setCrossfadeSec(double sec) {
    if (qFuzzyCompare(m_crossfadeSec, sec)) return;
    m_crossfadeSec = sec;
    emit settingsChanged();
}

void AudioSettingsController::setGaplessEnabled(bool en) {
    if (m_gaplessEnabled == en) return;
    m_gaplessEnabled = en;
    emit settingsChanged();
}

void AudioSettingsController::setReplayGainMode(const QString &mode) {
    if (m_replayGainMode == mode) return;
    m_replayGainMode = mode;
    emit settingsChanged();
}

void AudioSettingsController::setStreamingFormat(const QString &fmt) {
    if (m_streamingFormat == fmt) return;
    m_streamingFormat = fmt;
    emit settingsChanged();
}

QStringList AudioSettingsController::availableSampleRates() const {
    return {"44100 Hz", "48000 Hz", "96000 Hz (Hi-Res)", "192000 Hz (Master)"};
}

QStringList AudioSettingsController::availableBitDepths() const {
    return {"32-bit Float (Estudio)", "24-bit Lossless", "16-bit Estandar"};
}

QStringList AudioSettingsController::availableFormats() const {
    return {"Opus (192 kbps)", "AAC (192 kbps)", "MP3 (320 kbps)"};
}

} // namespace Dopamine::Features::Playback::App

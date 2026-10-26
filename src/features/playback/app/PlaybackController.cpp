/**
 * Location: src/features/playback/app/PlaybackController.cpp
 * Function: Implementation of library playback, transport controls, volume
 * persistence, and sibling .lrc lyric synchronization.
 * All Rights Reserved isgaar
 */
#include "PlaybackController.h"
#include "../../../core/audio/IAudioBackend.h"
#include <QFile>
#include <QFileInfo>
#include <QSettings>

namespace Dopamine::Features::Playback::App {

PlaybackController::PlaybackController(
    Core::Audio::IAudioBackend *aud, QObject *parent)
    : QObject(parent), m_audio(aud) {
    m_volume = QSettings("isgaar", "Dopamine")
        .value("Playback/Volume", 80).toInt();
    if (!m_audio) return;

    m_audio->setVolume(m_volume);
    connect(m_audio, &Core::Audio::IAudioBackend::stateChanged,
            this, &PlaybackController::onBackendStateChanged);
    connect(m_audio, &Core::Audio::IAudioBackend::positionChanged,
            this, &PlaybackController::onBackendPositionChanged);
    connect(m_audio, &Core::Audio::IAudioBackend::durationChanged,
            this, &PlaybackController::onBackendDurationChanged);
    connect(m_audio, &Core::Audio::IAudioBackend::errorOccurred, this,
            [this](const QString &) { if (m_audio) m_audio->pause(); });
}

bool PlaybackController::isPlaying() const {
    return m_audio && m_audio->isPlaying();
}

void PlaybackController::setVolume(int vol) {
    const int clamped = qBound(0, vol, 100);
    if (m_volume == clamped) return;
    m_volume = clamped;
    if (m_audio) m_audio->setVolume(m_volume);
    QSettings("isgaar", "Dopamine")
        .setValue("Playback/Volume", m_volume);
    emit volumeChanged();
}

void PlaybackController::togglePlay() {
    isPlaying() ? pause() : play();
}

void PlaybackController::play() {
    if (m_audio && m_currentUrl.isValid()) {
        m_audio->playAt(m_currentUrl, m_resumeMs);
        m_resumeMs = 0;
    }
}

void PlaybackController::pause() {
    m_resumeMs = static_cast<qint64>(m_position) * 1000;
    if (m_audio) m_audio->pause();
}

void PlaybackController::stop() {
    m_resumeMs = 0;
    if (m_audio) m_audio->stop();
    if (m_position != 0) {
        m_position = 0;
        emit positionChanged();
    }
}

void PlaybackController::playLibraryTrack(int id, const QString &title,
                                          const QString &artist,
                                          const QString &album,
                                          const QString &path, int dur) {
    m_track = {id, title, artist, album, QString{}, dur};
    m_currentUrl.clear();
    if (!path.isEmpty()) {
        m_currentUrl = path.startsWith('/') ?
            QUrl::fromLocalFile(path) : QUrl(path);
    }
    loadLyrics(path);
    m_position = 0;
    m_resumeMs = 0;
    m_activeLyric = -1;
    if (m_audio && m_currentUrl.isValid()) {
        m_audio->play(m_currentUrl);
    }
    emit trackChanged();
    emit positionChanged();
    emit lyricsChanged();
}

void PlaybackController::loadLibraryTrack(int id, const QString &title,
                                          const QString &artist,
                                          const QString &album,
                                          const QString &path, int dur,
                                          int positionSec) {
    if (m_audio) m_audio->stop();
    m_track = {id, title, artist, album, QString{}, dur};
    m_currentUrl.clear();
    if (!path.isEmpty()) {
        m_currentUrl = path.startsWith('/') ?
            QUrl::fromLocalFile(path) : QUrl(path);
    }
    loadLyrics(path);
    m_position = qMax(0, positionSec);
    m_resumeMs = static_cast<qint64>(m_position) * 1000;
    m_activeLyric = Domain::LrcParser::activeLineIndex(
        m_lyrics, m_resumeMs);
    emit trackChanged();
    emit positionChanged();
    emit lyricsChanged();
}

void PlaybackController::loadLyrics(const QString &trackPath) {
    m_lyrics.clear();
    m_lyricsVariant.clear();
    if (!trackPath.isEmpty() && !trackPath.contains("://")) {
        const QFileInfo info(trackPath);
        const QString lrcPath = info.absolutePath() + "/" +
            info.completeBaseName() + ".lrc";
        QFile file(lrcPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_lyrics = Domain::LrcParser::parse(
                QString::fromUtf8(file.readAll()));
        }
    }
    for (const auto &line : m_lyrics) {
        m_lyricsVariant.append(
            QVariantMap{{"timeMs", line.timeMs}, {"text", line.text}});
    }
}

void PlaybackController::onBackendStateChanged(bool) {
    emit playStateChanged();
}

void PlaybackController::onBackendPositionChanged(qint64 posMs) {
    m_position = static_cast<int>(posMs / 1000);
    emit positionChanged();
    const int idx = Domain::LrcParser::activeLineIndex(m_lyrics, posMs);
    if (idx != m_activeLyric) {
        m_activeLyric = idx;
        emit lyricsChanged();
    }
}

void PlaybackController::onBackendDurationChanged(qint64 durationMs) {
    const int seconds = static_cast<int>(durationMs / 1000);
    if (seconds > 0 && seconds != m_track.duration) {
        m_track.duration = seconds;
        emit positionChanged();
    }
}

} // namespace Dopamine::Features::Playback::App

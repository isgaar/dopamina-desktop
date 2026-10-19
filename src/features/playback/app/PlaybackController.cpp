/**
 * Location: src/features/playback/app/PlaybackController.cpp
 * Function: Master implementation of unified playback controller handling
 * radio tuning, stream lifecycle, track transitions, and metadata sync.
 * All Rights Reserved isgaar
 */
#include "PlaybackController.h"
#include "../infra/PlaybackApiClient.h"
#include "../infra/PlaybackSyncService.h"
#include "../../../core/audio/IAudioBackend.h"
#include "../../../core/ProjectConfig.h"
#include <QSettings>
#include <QUrl>
#include <QTimer>

namespace Dopamine::Features::Playback::App {
PlaybackController::PlaybackController(Core::ApiClient *api,
                                       Core::Audio::IAudioBackend *aud,
                                       QObject *parent)
    : QObject(parent), m_api(new Infra::PlaybackApiClient(api, this)),
      m_sync(new Infra::PlaybackSyncService(m_api, this)),
      m_audio(aud), m_syncTimer(new QTimer(this)) {
    m_volume = QSettings("isgaar", "Dopamine")
        .value("Playback/Volume", 80).toInt();
    if (m_audio) {
        m_audio->setVolume(m_volume);
        connect(m_audio, &Core::Audio::IAudioBackend::stateChanged,
                this, &PlaybackController::onBackendStateChanged);
        connect(m_audio, &Core::Audio::IAudioBackend::positionChanged,
                this, &PlaybackController::onBackendPositionChanged);
        connect(m_audio, &Core::Audio::IAudioBackend::icyMetadataReceived,
                this, &PlaybackController::onBackendIcyMetadata);
        connect(m_audio, &Core::Audio::IAudioBackend::errorOccurred, this,
                [this](const QString &) { if (m_audio) m_audio->pause(); });
    }
    connect(m_syncTimer, &QTimer::timeout,
            this, &PlaybackController::syncStationNowPlaying);
}
bool PlaybackController::isPlaying() const {
    return m_audio && m_audio->isPlaying();
}
int PlaybackController::volume() const { return m_volume; }
void PlaybackController::setVolume(int vol) {
    int c = qBound(0, vol, 100);
    if (m_volume == c) return;
    m_volume = c; if (m_audio) m_audio->setVolume(c);
    QSettings("isgaar", "Dopamine").setValue("Playback/Volume", c);
    emit volumeChanged();
}
void PlaybackController::tuneStation(const QString &id) {
    m_origin = "radio"; m_station.id = id;
    emit originChanged(); emit stationChanged();
    m_sync->tune(id, [this, id](bool ok, auto st, auto tr, auto l, auto v,
                                auto t, auto u) {
        if (!ok) return;
        m_station = st; m_track = tr; m_lyrics = l; m_lyricsVariant = v;
        m_stationTracks = t; m_upcomingTracks = u; m_activeLyric = -1;
        emit stationChanged(); emit trackChanged();
        emit lyricsChanged(); emit catalogChanged();
        if (m_audio) {
            auto url = Core::ProjectConfig::instance().streamUrl(id);
            m_audio->play(QUrl(url));
        }
        m_syncTimer->start(8000);
    });
}
void PlaybackController::togglePlay() { isPlaying() ? stop() : play(); }
void PlaybackController::play() {
    if (m_origin != "radio" || m_station.id.isEmpty() || !m_station.hasTracks)
        return;
    auto u = Core::ProjectConfig::instance().streamUrl(m_station.id);
    if (m_audio) m_audio->play(QUrl(u));
    m_syncTimer->start(8000);
}
void PlaybackController::stop() {
    if (m_audio) m_audio->stop();
    m_syncTimer->stop(); m_station.isLive = false; emit statusChanged();
}
void PlaybackController::skipNext() {
    if (!m_station.id.isEmpty())
        m_api->requestNext(m_station.id,
                           [this](bool) { syncStationNowPlaying(); });
}
void PlaybackController::setPlaybackOrigin(const QString &origin) {
    if (m_origin != origin) { m_origin = origin; emit originChanged(); }
}
void PlaybackController::playLibraryTrack(int id, const QString &tit,
                                         const QString &art, const QString &alb,
                                         const QString &path, int dur) {
    m_origin = "library"; m_track = {id, tit, art, alb, QString{}, dur};
    if (m_audio && !path.isEmpty()) {
        auto u = path.startsWith('/') ? QUrl::fromLocalFile(path) : QUrl(path);
        m_audio->play(u);
    }
    emit originChanged(); emit trackChanged();
}
void PlaybackController::onBackendStateChanged(bool p) {
    if (m_origin == "radio") { m_station.isLive = p; emit statusChanged(); }
    emit playStateChanged();
}
void PlaybackController::onBackendPositionChanged(qint64 posMs) {
    m_position = static_cast<int>(posMs / 1000); emit positionChanged();
    int idx = Domain::LrcParser::activeLineIndex(m_lyrics, posMs);
    if (idx != m_activeLyric) { m_activeLyric = idx; emit lyricsChanged(); }
}
void PlaybackController::onBackendIcyMetadata(const QString &a,
                                             const QString &t) {
    if (t.isEmpty()) return;
    m_track.title = t;
    if (!a.isEmpty()) m_track.artist = a;
    emit trackChanged();
}
void PlaybackController::syncStationNowPlaying() {
    m_sync->poll(m_station.id, m_track.id, [this](auto tr, auto l, auto v) {
        m_track = tr; m_lyrics = l; m_lyricsVariant = v; m_activeLyric = -1;
        emit trackChanged(); emit lyricsChanged();
    });
}
} // namespace Dopamine::Features::Playback::App
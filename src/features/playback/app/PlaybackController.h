/**
 * Location: src/features/playback/app/PlaybackController.h
 * Function: Single source of truth playback controller coordinating radio
 * and library streams, volume persistence, ICY metadata, and lyrics.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYBACK_APP_PLAYBACKCONTROLLER_H
#define DOPAMINE_FEATURES_PLAYBACK_APP_PLAYBACKCONTROLLER_H
#include <QObject>
#include <QVariantList>
#include "../domain/PlaybackTypes.h"
#include "../domain/LrcParser.h"
class QTimer;

namespace Dopamine::Core { class ApiClient; }

namespace Dopamine::Core::Audio { class IAudioBackend; }

namespace Dopamine::Features::Playback::Infra {
    class PlaybackApiClient;
    class PlaybackSyncService;
}

namespace Dopamine::Features::Playback::App {

class PlaybackController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentStationId READ stationId NOTIFY stationChanged)
    Q_PROPERTY(QString currentStationName READ stationName
               NOTIFY stationChanged)
    Q_PROPERTY(QString currentStationDial READ stationDial
               NOTIFY stationChanged)
    Q_PROPERTY(QString currentStationGenre READ stationGenre
               NOTIFY stationChanged)
    Q_PROPERTY(QString currentStationDesc READ stationDesc
               NOTIFY stationChanged)
    Q_PROPERTY(QString currentTrackTitle READ trackTitle NOTIFY trackChanged)
    Q_PROPERTY(QString currentTrackArtist READ trackArtist NOTIFY trackChanged)
    Q_PROPERTY(QString currentTrackAlbum READ trackAlbum NOTIFY trackChanged)
    Q_PROPERTY(QString currentTrackCover READ trackCover NOTIFY trackChanged)
    Q_PROPERTY(int duration READ duration NOTIFY positionChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playStateChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString playbackOrigin READ playbackOrigin NOTIFY originChanged)
    Q_PROPERTY(QString streamQuality READ streamQuality CONSTANT)
    Q_PROPERTY(bool isLive READ isLive NOTIFY statusChanged)
    Q_PROPERTY(bool hasTracks READ hasTracks NOTIFY statusChanged)
    Q_PROPERTY(int stationTrackCount READ trackCount NOTIFY statusChanged)
    Q_PROPERTY(bool lyricsAvailable READ lyricsAvailable NOTIFY lyricsChanged)
    Q_PROPERTY(int activeLyricIndex READ activeLyricIndex NOTIFY lyricsChanged)
    Q_PROPERTY(QVariantList lyricsLines READ lyricsLines NOTIFY lyricsChanged)
    Q_PROPERTY(QVariantList stationTracks READ stationTracks
               NOTIFY catalogChanged)
    Q_PROPERTY(QVariantList upcomingTracks READ upcomingTracks
               NOTIFY catalogChanged)

public:
    explicit PlaybackController(Core::ApiClient *apiClient,
                                Core::Audio::IAudioBackend *audioBackend,
                                QObject *parent = nullptr);
    ~PlaybackController() override = default;
    QString stationId() const { return m_station.id; }
    QString stationName() const { return m_station.name; }
    QString stationDial() const { return m_station.dial; }
    QString stationGenre() const { return m_station.genre; }
    QString stationDesc() const { return m_station.desc; }
    QString trackTitle() const { return m_track.title; }
    QString trackArtist() const { return m_track.artist; }
    QString trackAlbum() const { return m_track.album; }
    QString trackCover() const { return m_track.cover; }
    int duration() const { return m_track.duration; }
    int position() const { return m_position; }
    bool isPlaying() const;
    int volume() const;
    QString playbackOrigin() const { return m_origin; }
    QString streamQuality() const { return "Opus 128 kbps"; }
    bool isLive() const { return m_station.isLive; }
    bool hasTracks() const { return m_station.hasTracks; }
    int trackCount() const { return m_station.trackCount; }
    bool lyricsAvailable() const { return !m_lyrics.isEmpty(); }
    int activeLyricIndex() const { return m_activeLyric; }
    QVariantList lyricsLines() const { return m_lyricsVariant; }
    QVariantList stationTracks() const { return m_stationTracks; }
    QVariantList upcomingTracks() const { return m_upcomingTracks; }
    Q_INVOKABLE void tuneStation(const QString &stationId);
    Q_INVOKABLE void togglePlay(); Q_INVOKABLE void play();
    Q_INVOKABLE void stop(); Q_INVOKABLE void skipNext();
    Q_INVOKABLE void setVolume(int vol);
    Q_INVOKABLE void setPlaybackOrigin(const QString &origin);
    Q_INVOKABLE void playLibraryTrack(int id, const QString &tit,
                                      const QString &art, const QString &alb,
                                      const QString &cov, int dur);

signals:
    void stationChanged(); void trackChanged(); void positionChanged();
    void playStateChanged(); void volumeChanged(); void originChanged();
    void statusChanged(); void lyricsChanged(); void catalogChanged();
private slots:
    void onBackendStateChanged(bool playing);
    void onBackendPositionChanged(qint64 posMs);
    void onBackendIcyMetadata(const QString &artist, const QString &title);
    void syncStationNowPlaying();

private:
    Infra::PlaybackApiClient *m_api = nullptr;
    Infra::PlaybackSyncService *m_sync = nullptr;
    Core::Audio::IAudioBackend *m_audio = nullptr;
    QTimer *m_syncTimer = nullptr;
    Domain::ActiveStationInfo m_station;
    Domain::ActiveTrackInfo m_track;
    QString m_origin = "radio";
    int m_position = 0; int m_volume = 80; int m_activeLyric = -1;
    QList<Domain::LrcLine> m_lyrics; QVariantList m_lyricsVariant;
    QVariantList m_stationTracks; QVariantList m_upcomingTracks;
};
} // namespace Dopamine::Features::Playback::App
#endif // DOPAMINE_FEATURES_PLAYBACK_APP_PLAYBACKCONTROLLER_H

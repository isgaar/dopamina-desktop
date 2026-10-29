/**
 * Location: src/features/playback/app/PlaybackController.h
 * Function: Playback controller for library tracks, transport state, volume
 * persistence, and synchronized local LRC lyrics.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYBACK_APP_PLAYBACKCONTROLLER_H
#define DOPAMINE_FEATURES_PLAYBACK_APP_PLAYBACKCONTROLLER_H
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include "../domain/PlaybackTypes.h"
#include "../domain/LrcParser.h"

namespace Dopamine::Core::Audio { class IAudioBackend; }

namespace Dopamine::Features::Playback::App {

class PlaybackController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentTrackTitle READ trackTitle
               NOTIFY trackChanged)
    Q_PROPERTY(QString currentTrackArtist READ trackArtist
               NOTIFY trackChanged)
    Q_PROPERTY(QString currentTrackAlbum READ trackAlbum
               NOTIFY trackChanged)
    Q_PROPERTY(QString currentTrackCover READ trackCover
               NOTIFY trackChanged)
    Q_PROPERTY(int currentTrackId READ trackId NOTIFY trackChanged)
    Q_PROPERTY(QString currentTrackPath READ trackPath NOTIFY trackChanged)
    Q_PROPERTY(int duration READ duration NOTIFY positionChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playStateChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume
               NOTIFY volumeChanged)
    Q_PROPERTY(bool lyricsAvailable READ lyricsAvailable
               NOTIFY lyricsChanged)
    Q_PROPERTY(int activeLyricIndex READ activeLyricIndex
               NOTIFY lyricsChanged)
    Q_PROPERTY(QVariantList lyricsLines READ lyricsLines
               NOTIFY lyricsChanged)

public:
    explicit PlaybackController(Core::Audio::IAudioBackend *audioBackend,
                                QObject *parent = nullptr);
    ~PlaybackController() override = default;

    QString trackTitle() const { return m_track.title; }
    QString trackArtist() const { return m_track.artist; }
    QString trackAlbum() const { return m_track.album; }
    QString trackCover() const { return m_track.cover; }
    int trackId() const { return m_track.id; }
    QString trackPath() const { return m_currentUrl.toString(); }
    int duration() const { return m_track.duration; }
    int position() const { return m_position; }
    bool isPlaying() const;
    int volume() const { return m_volume; }
    bool lyricsAvailable() const { return !m_lyrics.isEmpty(); }
    int activeLyricIndex() const { return m_activeLyric; }
    QVariantList lyricsLines() const { return m_lyricsVariant; }

    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(int seconds);
    Q_INVOKABLE void setVolume(int vol);
    void seekFromRemote(int seconds);
    void setVolumeFromRemote(int vol);
    void syncRemoteProgress(int positionSec, int volumePct);
    Q_INVOKABLE void playLibraryTrack(int id, const QString &title,
                                      const QString &artist,
                                      const QString &album,
                                      const QString &path, int dur,
                                      const QString &coverUrl = QString{});
    Q_INVOKABLE void loadLibraryTrack(int id, const QString &title,
                                      const QString &artist,
                                      const QString &album,
                                      const QString &path, int dur,
                                      int positionSec,
                                      const QString &coverUrl = QString{});

signals:
    void trackChanged();
    void positionChanged();
    void playStateChanged();
    void volumeChanged();
    void userSeeked(int seconds);
    void userVolumeChanged(int vol);
    void lyricsChanged();

private slots:
    void onBackendStateChanged(bool playing);
    void onBackendPositionChanged(qint64 posMs);
    void onBackendDurationChanged(qint64 durationMs);

private:
    void loadLyrics(const QString &trackPath);

    Core::Audio::IAudioBackend *m_audio = nullptr;
    Domain::ActiveTrackInfo m_track;
    QUrl m_currentUrl;
    qint64 m_resumeMs = 0;
    int m_position = 0;
    int m_volume = 80;
    int m_activeLyric = -1;
    QList<Domain::LrcLine> m_lyrics;
    QVariantList m_lyricsVariant;
};
} // namespace Dopamine::Features::Playback::App
#endif // DOPAMINE_FEATURES_PLAYBACK_APP_PLAYBACKCONTROLLER_H

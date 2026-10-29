/**
 * Location: src/core/platform/media/mpris/MprisPlayerService.h
 * Function: MPRIS 2 D-Bus service coordinating playback metadata, transport
 * control commands, position tracking, and cover art exposure.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISPLAYERSERVICE_H
#define DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISPLAYERSERVICE_H

#include <QObject>
#include <QVariantMap>
#include <QDBusObjectPath>

namespace Dopamine::Features::Playback::App { class PlaybackController; }
namespace Dopamine::Features::Queue::App { class QueueController; }

namespace Dopamine::Core::Platform::Media::Mpris {

class MprisRootAdaptor;
class MprisPlayerAdaptor;

class MprisPlayerService : public QObject {
    Q_OBJECT

public:
    explicit MprisPlayerService(
        Features::Playback::App::PlaybackController *playbackCtrl,
        Features::Queue::App::QueueController *queueCtrl,
        QObject *parent = nullptr);
    ~MprisPlayerService() override;

    bool registerService();

    // MPRIS Player Properties
    QString playbackStatus() const;
    QString loopStatus() const;
    void setLoopStatus(const QString &val);
    double rate() const;
    void setRate(double val);
    bool shuffle() const;
    void setShuffle(bool val);
    QVariantMap metadata() const;
    double volume() const;
    void setVolume(double val);
    qlonglong position() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;

    // MPRIS Actions
    void next();
    void previous();
    void pause();
    void playPause();
    void stop();
    void play();
    void seek(qlonglong offsetUs);
    void setPosition(const QDBusObjectPath &trackId, qlonglong positionUs);
    void openUri(const QString &uri);
    void raise();
    void quit();

signals:
    void raiseRequested();
    void quitRequested();

private slots:
    void onTrackChanged();
    void onPlayStateChanged();
    void onVolumeChanged();
    void onUserSeeked(int seconds);
    void onQueueChanged();
    void onShuffleChanged();
    void onRepeatChanged();

private:
    void updateMetadata();
    void notifyPropertiesChanged(const QVariantMap &changed);
    QString resolveArtUrl(const QString &coverUrl, int trackId) const;

    Features::Playback::App::PlaybackController *m_playbackCtrl = nullptr;
    Features::Queue::App::QueueController *m_queueCtrl = nullptr;
    MprisRootAdaptor *m_rootAdaptor = nullptr;
    MprisPlayerAdaptor *m_playerAdaptor = nullptr;
    QString m_registeredServiceName;
    QVariantMap m_metadata;
    QString m_playbackStatus;
    bool m_isExplicitlyStopped = false;
};

} // namespace Dopamine::Core::Platform::Media::Mpris

#endif // DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISPLAYERSERVICE_H

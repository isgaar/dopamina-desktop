/**
 * Location: src/features/interconnect/app/SessionPlaybackController.h
 * Function: Polls the newest remote playback session, resolves its queue ids,
 * mirrors the queue into the desktop player, and enables real-time remote control.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_INTERCONNECT_APP_SESSIONPLAYBACKCONTROLLER_H
#define DOPAMINE_FEATURES_INTERCONNECT_APP_SESSIONPLAYBACKCONTROLLER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include "../infra/SessionPlaybackApiClient.h"

class QTimer;

namespace Dopamine::Features::Queue::App { class QueueController; }

namespace Dopamine::Features::Interconnect::App {

class SessionPlaybackController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hasSession READ hasSession NOTIFY sessionChanged)
    Q_PROPERTY(QString sourceKind READ sourceKind NOTIFY sessionChanged)
    Q_PROPERTY(bool remotePlaying READ remotePlaying NOTIFY sessionChanged)
    Q_PROPERTY(bool localPlaying READ localPlaying NOTIFY sessionChanged)
    Q_PROPERTY(bool isControllingAndroid READ isControllingAndroid
               NOTIFY sessionChanged)
    Q_PROPERTY(QString activeDeviceKind READ activeDeviceKind
               NOTIFY sessionChanged)
    Q_PROPERTY(bool canSwitchToDesktop READ canSwitchToDesktop
               NOTIFY sessionChanged)
    Q_PROPERTY(bool canSwitchToAndroid READ canSwitchToAndroid
               NOTIFY sessionChanged)
    Q_PROPERTY(int queueSize READ queueSize NOTIFY sessionChanged)
    Q_PROPERTY(double remoteVolume READ remoteVolume NOTIFY sessionChanged)
    Q_PROPERTY(int remoteVolumePct READ remoteVolumePct NOTIFY sessionChanged)

public:
    explicit SessionPlaybackController(
        Infra::SessionPlaybackApiClient *api,
        Queue::App::QueueController *queue,
        QObject *parent = nullptr);

    bool hasSession() const { return m_hasSession || m_localTrackId > 0; }
    QString sourceKind() const { return m_sourceKind; }
    bool remotePlaying() const { return m_remotePlaying; }
    bool localPlaying() const { return m_localPlaying; }
    bool isControllingAndroid() const {
        return !m_localPlaying && m_sourceKind == QLatin1String("mobile");
    }
    QString activeDeviceKind() const {
        if (m_localPlaying) return QStringLiteral("desktop");
        if (m_remotePlaying && !m_sourceKind.isEmpty()) return m_sourceKind;
        if (!m_sourceKind.isEmpty()) return m_sourceKind;
        return QStringLiteral("desktop");
    }
    bool canSwitchToDesktop() const {
        return !m_localPlaying && (m_hasSession || m_remotePlaying);
    }
    bool canSwitchToAndroid() const {
        return m_localPlaying || m_localTrackId > 0 || m_queueSize > 0;
    }
    int queueSize() const { return m_queueSize; }
    double remoteVolume() const { return m_localVolume / 100.0; }
    int remoteVolumePct() const { return m_localVolume; }

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void refreshNow();
    Q_INVOKABLE void switchToDesktop();
    Q_INVOKABLE void switchToAndroid();
    Q_INVOKABLE void toggleRemotePlay();
    Q_INVOKABLE void remoteNext();
    Q_INVOKABLE void remotePrevious();
    Q_INVOKABLE void remoteSeek(int posSec);
    Q_INVOKABLE void remoteSetVolume(int volumePct);
    void syncLocalPlayback(int trackId, int positionSec, bool playing,
                           int volume);
    void sendRemoteCommandToAndroid(int trackId, int positionSec, bool playing,
                                    int volume);

signals:
    void sessionChanged();
    void takeoverDesktopRequested();
    void pauseLocalRequested();
    void remoteProgressUpdated(int positionSec, int volumePct, bool playing);
    void remoteSeekCommandReceived(int positionSec);
    void remoteVolumeCommandReceived(int volumePct);

private:
    void poll();
    void handleStates(quint64 generation);
    void handleRemoteCommandToDesktop(const QJsonObject &state,
                                      quint64 generation);
    void loadSavedSessions(quint64 generation);
    void handleSavedSessions(quint64 generation);
    void applyState(const QString &userId, const QJsonObject &state,
                    quint64 generation);
    void applyQueue(const QString &userId, const QJsonArray &queue,
                    int trackId, int positionSec, bool playing,
                    quint64 generation, bool autoPlayDesktop = false);
    void updateSession(bool active, const QString &source, bool playing,
                       int size);

    Infra::SessionPlaybackApiClient *m_api = nullptr;
    Queue::App::QueueController *m_queue = nullptr;
    QTimer *m_timer = nullptr;
    QList<QPair<QString, QJsonObject>> m_candidates;
    QString m_lastSignature;
    QString m_sourceKind;
    quint64 m_generation = 0;
    qint64 m_lastRemoteCmdSentMs = 0;
    double m_lastLocalPlayAt = 0.0;
    int m_pending = 0;
    int m_okCount = 0;
    int m_queueSize = 0;
    int m_localTrackId = 0;
    int m_localPositionSec = 0;
    int m_localVolume = 80;
    bool m_hasSession = false;
    bool m_remotePlaying = false;
    bool m_localPlaying = false;
    bool m_remoteControlMobile = false;
    bool m_savedChecked = false;
    bool m_resolving = false;
};

} // namespace Dopamine::Features::Interconnect::App

#endif // DOPAMINE_FEATURES_INTERCONNECT_APP_SESSIONPLAYBACKCONTROLLER_H

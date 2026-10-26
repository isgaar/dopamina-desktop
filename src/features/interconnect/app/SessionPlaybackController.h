/**
 * Location: src/features/interconnect/app/SessionPlaybackController.h
 * Function: Polls the newest remote playback session, resolves its queue ids,
 * and mirrors the queue into the desktop player without stealing focus.
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
    Q_PROPERTY(int queueSize READ queueSize NOTIFY sessionChanged)

public:
    explicit SessionPlaybackController(
        Infra::SessionPlaybackApiClient *api,
        Queue::App::QueueController *queue,
        QObject *parent = nullptr);

    bool hasSession() const { return m_hasSession; }
    QString sourceKind() const { return m_sourceKind; }
    bool remotePlaying() const { return m_remotePlaying; }
    int queueSize() const { return m_queueSize; }

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void refreshNow();

signals:
    void sessionChanged();

private:
    void poll();
    void handleStates(quint64 generation);
    void loadSavedSessions(quint64 generation);
    void handleSavedSessions(quint64 generation);
    void applyState(const QString &userId, const QJsonObject &state,
                    quint64 generation);
    void applyQueue(const QString &userId, const QJsonArray &queue,
                    int trackId, int positionSec, bool playing,
                    quint64 generation);
    void updateSession(bool active, const QString &source, bool playing,
                       int size);

    Infra::SessionPlaybackApiClient *m_api = nullptr;
    Queue::App::QueueController *m_queue = nullptr;
    QTimer *m_timer = nullptr;
    QList<QPair<QString, QJsonObject>> m_candidates;
    QString m_lastSignature;
    QString m_sourceKind;
    quint64 m_generation = 0;
    int m_pending = 0;
    int m_okCount = 0;
    int m_queueSize = 0;
    bool m_hasSession = false;
    bool m_remotePlaying = false;
    bool m_savedChecked = false;
    bool m_resolving = false;
};

} // namespace Dopamine::Features::Interconnect::App

#endif // DOPAMINE_FEATURES_INTERCONNECT_APP_SESSIONPLAYBACKCONTROLLER_H

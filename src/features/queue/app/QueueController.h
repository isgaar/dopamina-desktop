/**
 * Location: src/features/queue/app/QueueController.h
 * Function: Queue coordinator managing play-next insertions, shuffle/repeat
 * modes, song mix expansion, and playback progression tracking.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_QUEUE_APP_QUEUECONTROLLER_H
#define DOPAMINE_FEATURES_QUEUE_APP_QUEUECONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QList>
#include "../infra/QueueApiClient.h"

namespace Dopamine::Features::Queue::App {

class QueueController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList queueTracks READ queueTracks NOTIFY queueChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY indexChanged)
    Q_PROPERTY(bool isMixActive READ isMixActive NOTIFY mixChanged)
    Q_PROPERTY(QString mixSeedTitle READ mixSeedTitle NOTIFY mixChanged)
    Q_PROPERTY(bool isShuffle READ isShuffle NOTIFY shuffleChanged)
    Q_PROPERTY(bool isRepeat READ isRepeat NOTIFY repeatChanged)

public:
    explicit QueueController(Infra::QueueApiClient *apiClient,
                           QObject *parent = nullptr);

    QVariantList queueTracks() const { return m_queue; }
    int currentIndex() const { return m_currentIndex; }
    bool isMixActive() const { return m_isMixActive; }
    QString mixSeedTitle() const { return m_mixSeedTitle; }
    bool isShuffle() const { return m_isShuffle; }
    bool isRepeat() const { return m_isRepeat; }

    Q_INVOKABLE void playNow(const QVariantMap &track);
    Q_INVOKABLE void playNext(const QVariantMap &track);
    Q_INVOKABLE void appendToQueue(const QVariantMap &track);
    Q_INVOKABLE void clearQueue();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void startMix(int seedTrackId, const QString &seedTitle);
    Q_INVOKABLE void toggleShuffle();
    Q_INVOKABLE void toggleRepeat();
    Q_INVOKABLE void applySessionQueue(const QVariantList &tracks,
                                       int currentIndex, int positionSec);

signals:
    void queueChanged();
    void indexChanged();
    void mixChanged();
    void shuffleChanged();
    void repeatChanged();
    void playTrackRequested(int id, const QString &title,
                            const QString &artist, const QString &album,
                            const QString &path, int duration);
    void sessionTrackRequested(int id, const QString &title,
                               const QString &artist, const QString &album,
                               const QString &path, int duration,
                               int positionSec);

private:
    void refillMixIfNeeded();
    void playCurrentTrack();

    Infra::QueueApiClient *m_api = nullptr;
    QVariantList m_queue;
    QList<int> m_history;
    int m_currentIndex = -1;
    int m_seedId = 0;
    QString m_mixSeedTitle;
    bool m_isMixActive = false;
    bool m_isShuffle = false;
    bool m_isRepeat = false;
};

} // namespace Dopamine::Features::Queue::App

#endif // DOPAMINE_FEATURES_QUEUE_APP_QUEUECONTROLLER_H

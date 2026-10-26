/**
 * Location: src/features/queue/app/QueueController.cpp
 * Function: Implementation of player queue controller orchestrating dynamic
 * track lists, repeat/shuffle algorithms, and song mix queues.
 * All Rights Reserved isgaar
 */
#include "QueueController.h"

namespace Dopamine::Features::Queue::App {

QueueController::QueueController(Infra::QueueApiClient *apiClient,
                                 QObject *parent)
    : QObject(parent), m_api(apiClient) {}

void QueueController::playNow(const QVariantMap &track) {
    m_queue.insert(m_currentIndex + 1, track);
    m_currentIndex++;
    emit queueChanged(); emit indexChanged();
    playCurrentTrack();
}

void QueueController::playNext(const QVariantMap &track) {
    m_queue.insert(m_currentIndex + 1, track);
    emit queueChanged();
}

void QueueController::appendToQueue(const QVariantMap &track) {
    m_queue.append(track);
    emit queueChanged();
}

void QueueController::clearQueue() {
    m_queue.clear(); m_currentIndex = -1;
    emit queueChanged(); emit indexChanged();
}

void QueueController::applySessionQueue(const QVariantList &tracks,
                                        int currentIndex, int positionSec) {
    if (tracks.isEmpty()) return;
    m_queue = tracks;
    m_currentIndex = qBound(0, currentIndex, tracks.size() - 1);
    m_history.clear();
    const bool hadMix = m_isMixActive;
    m_isMixActive = false;
    m_seedId = 0;
    m_mixSeedTitle.clear();
    if (hadMix) emit mixChanged();
    emit queueChanged();
    emit indexChanged();

    const QVariantMap track = m_queue[m_currentIndex].toMap();
    const int id = track.value(QStringLiteral("id")).toInt();
    if (id <= 0) return;
    QString title = track.value(QStringLiteral("title")).toString();
    if (title.isEmpty())
        title = track.value(QStringLiteral("titulo")).toString();
    QString artist = track.value(QStringLiteral("artist")).toString();
    if (artist.isEmpty())
        artist = track.value(QStringLiteral("artista")).toString();
    int duration = track.value(QStringLiteral("duration")).toInt();
    if (duration <= 0)
        duration = track.value(QStringLiteral("duracion")).toInt();
    emit sessionTrackRequested(
        id, title, artist, track.value(QStringLiteral("album")).toString(),
        track.value(QStringLiteral("path")).toString(), duration,
        qMax(0, positionSec));
}

void QueueController::toggleShuffle() {
    m_isShuffle = !m_isShuffle; emit shuffleChanged();
}

void QueueController::toggleRepeat() {
    m_isRepeat = !m_isRepeat; emit repeatChanged();
}

void QueueController::next() {
    if (m_queue.isEmpty()) return;
    if (m_currentIndex + 1 < m_queue.size()) {
        m_currentIndex++;
        emit indexChanged();
        playCurrentTrack();
        if (m_isMixActive) refillMixIfNeeded();
    } else if (m_isRepeat) {
        m_currentIndex = 0;
        emit indexChanged();
        playCurrentTrack();
    }
}

void QueueController::previous() {
    if (m_currentIndex > 0) {
        m_currentIndex--;
        emit indexChanged();
        playCurrentTrack();
    }
}

void QueueController::startMix(int seedTrackId, const QString &seedTitle) {
    m_isMixActive = true; m_seedId = seedTrackId;
    m_mixSeedTitle = seedTitle; emit mixChanged();
    m_queue.clear(); m_currentIndex = 0;
    QVariantMap seedTrack;
    seedTrack["id"] = seedTrackId;
    seedTrack["title"] = seedTitle;
    m_queue.append(seedTrack);
    emit queueChanged(); emit indexChanged();
    playCurrentTrack();
    refillMixIfNeeded();
}

void QueueController::refillMixIfNeeded() {
    if (!m_api || !m_isMixActive) return;
    int remaining = m_queue.size() - m_currentIndex;
    if (remaining >= 5) return;
    m_api->fetchMixNext(m_seedId, m_history, {}, "normal", 25,
        [this](bool ok, auto ids) {
            if (!ok) return;
            for (const auto &val : ids) {
                QVariantMap item;
                item["id"] = val.toInt();
                item["title"] = QString("Pista #%1").arg(val.toInt());
                item["isMix"] = true;
                m_queue.append(item);
            }
            emit queueChanged();
        });
}

void QueueController::playCurrentTrack() {
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) return;
    QVariantMap t = m_queue[m_currentIndex].toMap();
    int id = t["id"].toInt();
    if (id <= 0) return;
    m_history.append(id);
    if (m_history.size() > 50) m_history.removeFirst();
    if (m_api) m_api->recordHistory(id);
    emit playTrackRequested(
        id, t["title"].toString(), t["artist"].toString(),
        t["album"].toString(), t["path"].toString(),
        t["duration"].toInt());
}

} // namespace Dopamine::Features::Queue::App

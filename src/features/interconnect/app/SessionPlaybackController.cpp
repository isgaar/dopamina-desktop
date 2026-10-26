/**
 * Location: src/features/interconnect/app/SessionPlaybackController.cpp
 * Function: Polls playback state for web and mobile clients, resolves the
 * newest active queue, and mirrors it into the desktop queue in pause.
 * All Rights Reserved isgaar
 */
#include "SessionPlaybackController.h"
#include "../../queue/app/QueueController.h"
#include <QHash>
#include <QJsonValue>
#include <QTimer>
#include <QVariantMap>

namespace {

constexpr int kPollMs = 5000;

QString sourceKindFor(const QString &userId) {
    return userId == QLatin1String("mobile_client")
        ? QStringLiteral("mobile") : QStringLiteral("web");
}

QVariantMap toTrackMap(const QJsonObject &object) {
    QVariantMap track = object.toVariantMap();
    QString stream = object.value(QStringLiteral("streamUrl")).toString();
    if (stream.isEmpty())
        stream = object.value(QStringLiteral("stream_url")).toString();
    track.insert(QStringLiteral("path"), stream);
    if (track.value(QStringLiteral("duration")).toInt() <= 0) {
        track.insert(QStringLiteral("duration"),
                     track.value(QStringLiteral("duracion")));
    }
    return track;
}

} // namespace

namespace Dopamine::Features::Interconnect::App {

SessionPlaybackController::SessionPlaybackController(
    Infra::SessionPlaybackApiClient *api,
    Queue::App::QueueController *queue,
    QObject *parent)
    : QObject(parent), m_api(api), m_queue(queue) {
    m_timer = new QTimer(this);
    m_timer->setInterval(kPollMs);
    connect(m_timer, &QTimer::timeout,
            this, &SessionPlaybackController::poll);
}

void SessionPlaybackController::start() {
    if (!m_api || !m_queue) return;
    m_savedChecked = false;
    if (!m_timer->isActive()) m_timer->start();
    poll();
}

void SessionPlaybackController::stop() {
    m_timer->stop();
    ++m_generation;
    m_pending = 0;
    m_resolving = false;
    m_candidates.clear();
    m_lastSignature.clear();
    updateSession(false, {}, false, 0);
}

void SessionPlaybackController::refreshNow() {
    if (!m_timer->isActive()) { start(); return; }
    poll();
}

void SessionPlaybackController::poll() {
    if (m_pending > 0 || m_resolving) return;
    const quint64 generation = ++m_generation;
    m_candidates.clear();
    m_okCount = 0;
    m_pending = 2;
    const QStringList users = {QStringLiteral("default"),
                               QStringLiteral("mobile_client")};
    for (const QString &userId : users) {
        m_api->fetchState(userId,
            [this, generation, userId](bool ok, const QJsonObject &state) {
                if (generation != m_generation) return;
                if (ok) {
                    ++m_okCount;
                    if (!state.isEmpty())
                        m_candidates.append({userId, state});
                }
                --m_pending;
                if (m_pending == 0) handleStates(generation);
            });
    }
}

void SessionPlaybackController::handleStates(quint64 generation) {
    if (m_okCount == 0) return;
    QJsonObject best;
    QString bestUser;
    double bestAt = -1.0;
    for (const auto &candidate : m_candidates) {
        const QJsonObject state = candidate.second;
        const QJsonArray queue =
            state.value(QStringLiteral("active_queue")).toArray();
        const int trackId = state.value(QStringLiteral("track_id")).toInt();
        if (queue.isEmpty() && trackId <= 0) continue;
        const double updatedAt =
            state.value(QStringLiteral("updated_at")).toDouble();
        if (updatedAt >= bestAt) {
            bestAt = updatedAt;
            best = state;
            bestUser = candidate.first;
        }
    }
    if (!best.isEmpty()) {
        applyState(bestUser, best, generation);
        return;
    }
    if (!m_savedChecked) {
        m_savedChecked = true;
        loadSavedSessions(generation);
        return;
    }
    m_lastSignature.clear();
    updateSession(false, {}, false, 0);
}

void SessionPlaybackController::loadSavedSessions(quint64 generation) {
    m_candidates.clear();
    m_okCount = 0;
    m_pending = 2;
    const QStringList users = {QStringLiteral("default"),
                               QStringLiteral("mobile_client")};
    for (const QString &userId : users) {
        m_api->fetchSavedSession(userId,
            [this, generation, userId](bool ok, const QJsonObject &session) {
                if (generation != m_generation) return;
                if (ok) {
                    ++m_okCount;
                    if (!session.isEmpty())
                        m_candidates.append({userId, session});
                }
                --m_pending;
                if (m_pending == 0) handleSavedSessions(generation);
            });
    }
}

void SessionPlaybackController::handleSavedSessions(quint64 generation) {
    if (m_okCount == 0) return;
    QJsonObject best;
    QString bestUser;
    double bestAt = -1.0;
    for (const auto &candidate : m_candidates) {
        const QJsonObject session = candidate.second;
        const QJsonArray queue =
            session.value(QStringLiteral("active_queue")).toArray();
        if (queue.isEmpty()) continue;
        const double savedAt =
            session.value(QStringLiteral("saved_at")).toDouble();
        if (savedAt >= bestAt) {
            bestAt = savedAt;
            best = session;
            bestUser = candidate.first;
        }
    }
    if (best.isEmpty()) {
        m_lastSignature.clear();
        updateSession(false, {}, false, 0);
        return;
    }
    applyQueue(bestUser,
               best.value(QStringLiteral("active_queue")).toArray(),
               best.value(QStringLiteral("track_id")).toInt(),
               qRound(best.value(QStringLiteral("position_sec")).toDouble()),
               false, generation);
}

void SessionPlaybackController::applyState(const QString &userId,
                                           const QJsonObject &state,
                                           quint64 generation) {
    QJsonArray queue = state.value(QStringLiteral("active_queue")).toArray();
    const int trackId = state.value(QStringLiteral("track_id")).toInt();
    if (queue.isEmpty() && trackId > 0) queue.append(trackId);
    applyQueue(userId, queue, trackId,
               qRound(state.value(QStringLiteral("position_sec")).toDouble()),
               state.value(QStringLiteral("is_playing")).toBool(),
               generation);
}

void SessionPlaybackController::applyQueue(const QString &userId,
                                           const QJsonArray &queue,
                                           int trackId, int positionSec,
                                           bool playing, quint64 generation) {
    QList<int> ids;
    QStringList tokens;
    for (const QJsonValue &value : queue) {
        const int id = value.toInt();
        if (id <= 0) continue;
        ids.append(id);
        tokens.append(QString::number(id));
    }
    if (ids.isEmpty()) return;

    const QString signature = userId + QLatin1Char('|') +
        QString::number(trackId) + QLatin1Char('|') + tokens.join(',');
    if (signature == m_lastSignature) {
        updateSession(true, sourceKindFor(userId), playing, ids.size());
        return;
    }

    m_resolving = true;
    m_api->resolveTracks(ids,
        [this, generation, signature, userId, trackId, positionSec, playing,
         ids](bool ok, const QJsonArray &tracks) {
            if (generation != m_generation) return;
            m_resolving = false;
            if (!ok) return;
            QHash<int, QVariantMap> byId;
            for (const QJsonValue &value : tracks) {
                const QJsonObject object = value.toObject();
                const int id = object.value(QStringLiteral("id")).toInt();
                if (id > 0) byId.insert(id, toTrackMap(object));
            }
            QVariantList resolved;
            int currentIndex = -1;
            for (int id : ids) {
                const auto it = byId.constFind(id);
                if (it == byId.constEnd()) continue;
                if (id == trackId && currentIndex < 0)
                    currentIndex = resolved.size();
                resolved.append(*it);
            }
            if (resolved.isEmpty()) return;
            if (currentIndex < 0) currentIndex = 0;
            m_queue->applySessionQueue(resolved, currentIndex, positionSec);
            m_lastSignature = signature;
            updateSession(true, sourceKindFor(userId), playing,
                          resolved.size());
        });
}

void SessionPlaybackController::updateSession(bool active,
                                              const QString &source,
                                              bool playing, int size) {
    if (m_hasSession == active && m_sourceKind == source &&
        m_remotePlaying == playing && m_queueSize == size) {
        return;
    }
    m_hasSession = active;
    m_sourceKind = source;
    m_remotePlaying = playing;
    m_queueSize = size;
    emit sessionChanged();
}

} // namespace Dopamine::Features::Interconnect::App

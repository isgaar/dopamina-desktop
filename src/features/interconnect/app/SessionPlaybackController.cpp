/**
 * Location: src/features/interconnect/app/SessionPlaybackController.cpp
 * Function: Polls playback state for web and mobile clients, resolves the
 * newest active queue, mirrors it into the desktop queue, and handles
 * bi-directional real-time remote control between Desktop and Android.
 * All Rights Reserved isgaar
 */
#include "SessionPlaybackController.h"
#include "../../queue/app/QueueController.h"
#include <QDateTime>
#include <QHash>
#include <QJsonValue>
#include <QTimer>
#include <QVariantMap>

namespace {

constexpr int kPollMs = 1200;

QString sourceKindFor(const QString &userId) {
    return userId == QLatin1String("mobile_client")
        ? QStringLiteral("mobile") : QStringLiteral("web");
}

QVariantMap toTrackMap(const QJsonObject &object) {
    QVariantMap track = object.toVariantMap();
    const int id = object.value(QStringLiteral("id")).toInt();
    QString stream = object.value(QStringLiteral("streamUrl")).toString();
    if (stream.isEmpty())
        stream = object.value(QStringLiteral("stream_url")).toString();
    track.insert(QStringLiteral("path"), stream);
    QString cover = object.value(QStringLiteral("coverUrl")).toString();
    if (cover.isEmpty())
        cover = object.value(QStringLiteral("cover_url")).toString();
    if (cover.isEmpty() && id > 0 && stream.contains(QLatin1String("/api/v1/"))) {
        const int idx = stream.indexOf(QLatin1String("/api/v1/"));
        cover = stream.left(idx) + QStringLiteral("/api/v1/covers/%1").arg(id);
    }
    if (!cover.isEmpty()) {
        track.insert(QStringLiteral("coverUrl"), cover);
        track.insert(QStringLiteral("cover"), cover);
    }
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
    m_remoteControlMobile = false;
    m_localTrackId = 0;
    m_localPositionSec = 0;
    m_localPlaying = false;
    m_candidates.clear();
    m_lastSignature.clear();
    updateSession(false, {}, false, 0);
}

void SessionPlaybackController::refreshNow() {
    if (!m_timer->isActive()) { start(); return; }
    poll();
}

void SessionPlaybackController::syncLocalPlayback(int trackId, int positionSec,
                                                  bool playing, int volume) {
    const bool changed = (m_localPlaying != playing || m_localTrackId != trackId);
    m_localTrackId = trackId;
    m_localPositionSec = qMax(0, positionSec);
    m_localPlaying = playing;
    m_localVolume = qBound(0, volume, 100);
    if (playing) {
        m_sourceKind = QStringLiteral("desktop");
        m_remotePlaying = false;
        m_remoteControlMobile = false;
        m_lastLocalPlayAt = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    }
    if (changed) emit sessionChanged();
    if (!m_api || trackId <= 0) return;
    if (!playing && isControllingAndroid()) return;

    QJsonArray qIds;
    if (m_queue) {
        for (const QVariant &v : m_queue->queueTracks()) {
            const int id = v.toMap().value(QStringLiteral("id")).toInt();
            if (id > 0) qIds.append(id);
        }
    }
    if (qIds.isEmpty()) qIds.append(trackId);
    QJsonObject state{
        {QStringLiteral("user_id"), QStringLiteral("default")},
        {QStringLiteral("track_id"), trackId},
        {QStringLiteral("position_sec"), m_localPositionSec},
        {QStringLiteral("is_playing"), playing},
        {QStringLiteral("active_queue"), qIds},
        {QStringLiteral("volume"), m_localVolume / 100.0},
        {QStringLiteral("device_id"), QStringLiteral("desktop_client")}
    };
    m_api->publishState(state);
}

void SessionPlaybackController::sendRemoteCommandToAndroid(int trackId,
                                                           int positionSec,
                                                           bool playing,
                                                           int volume) {
    m_localTrackId = trackId > 0 ? trackId : m_localTrackId;
    m_localPositionSec = qMax(0, positionSec);
    m_remotePlaying = playing;
    m_localVolume = qBound(0, volume, 100);
    m_sourceKind = QStringLiteral("mobile");
    m_remoteControlMobile = true;
    m_lastRemoteCmdSentMs = QDateTime::currentMSecsSinceEpoch();

    if (m_api && m_localTrackId > 0) {
        QJsonArray qIds;
        if (m_queue) {
            for (const QVariant &v : m_queue->queueTracks()) {
                const int id = v.toMap().value(QStringLiteral("id")).toInt();
                if (id > 0) qIds.append(id);
            }
        }
        if (qIds.isEmpty()) qIds.append(m_localTrackId);
        QJsonObject mobileCmd{
            {QStringLiteral("user_id"), QStringLiteral("mobile_client")},
            {QStringLiteral("track_id"), m_localTrackId},
            {QStringLiteral("position_sec"), m_localPositionSec},
            {QStringLiteral("is_playing"), playing},
            {QStringLiteral("active_queue"), qIds},
            {QStringLiteral("volume"), m_localVolume / 100.0},
            {QStringLiteral("device_id"), QStringLiteral("remote_cmd_to_mobile")}
        };
        m_api->publishState(mobileCmd);
    }
    emit sessionChanged();
}

void SessionPlaybackController::toggleRemotePlay() {
    sendRemoteCommandToAndroid(m_localTrackId, m_localPositionSec,
                               !m_remotePlaying, m_localVolume);
}

void SessionPlaybackController::switchToDesktop() {
    if (m_api) {
        QJsonObject pauseMobile{
            {QStringLiteral("user_id"), QStringLiteral("mobile_client")},
            {QStringLiteral("is_playing"), false},
            {QStringLiteral("device_id"), QStringLiteral("desktop_client")}
        };
        m_api->publishState(pauseMobile);
    }
    m_remoteControlMobile = false;
    m_remotePlaying = false;
    m_localPlaying = true;
    m_sourceKind = QStringLiteral("desktop");
    m_lastLocalPlayAt = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    emit takeoverDesktopRequested();
    emit sessionChanged();
}

void SessionPlaybackController::switchToAndroid() {
    emit pauseLocalRequested();
    m_localPlaying = false;
    m_remotePlaying = true;
    m_remoteControlMobile = true;
    m_hasSession = true;
    m_sourceKind = QStringLiteral("mobile");
    m_lastRemoteCmdSentMs = QDateTime::currentMSecsSinceEpoch();
    if (m_api) {
        QJsonArray qIds;
        int currentId = m_localTrackId;
        if (m_queue) {
            const QVariantList tracks = m_queue->queueTracks();
            for (const QVariant &v : tracks) {
                const int id = v.toMap().value(QStringLiteral("id")).toInt();
                if (id > 0) qIds.append(id);
            }
            const int idx = m_queue->currentIndex();
            if (currentId <= 0 && idx >= 0 && idx < tracks.size()) {
                currentId = tracks[idx].toMap().value(QStringLiteral("id")).toInt();
            }
        }
        if (qIds.isEmpty() && currentId > 0) qIds.append(currentId);
        QJsonObject mobileState{
            {QStringLiteral("user_id"), QStringLiteral("mobile_client")},
            {QStringLiteral("track_id"), currentId},
            {QStringLiteral("position_sec"), m_localPositionSec},
            {QStringLiteral("is_playing"), true},
            {QStringLiteral("active_queue"), qIds},
            {QStringLiteral("volume"), m_localVolume / 100.0},
            {QStringLiteral("device_id"), QStringLiteral("transfer_to_mobile")}
        };
        m_api->publishState(mobileState);
        m_api->saveSession(QJsonObject{
            {QStringLiteral("user_id"), QStringLiteral("mobile_client")},
            {QStringLiteral("track_id"), currentId},
            {QStringLiteral("position_sec"), m_localPositionSec},
            {QStringLiteral("active_queue"), qIds}
        });
    }
    emit sessionChanged();
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

void SessionPlaybackController::handleRemoteCommandToDesktop(
    const QJsonObject &state, quint64 generation) {
    const int cmdTrackId = state.value(QStringLiteral("track_id")).toInt();
    const int cmdPosSec =
        qRound(state.value(QStringLiteral("position_sec")).toDouble());
    const bool cmdPlaying =
        state.value(QStringLiteral("is_playing")).toBool(m_localPlaying);
    const QJsonValue volVal = state.value(QStringLiteral("volume"));
    const int cmdVolume = (!volVal.isUndefined() && !volVal.isNull())
        ? qBound(0, qRound(volVal.toDouble() * 100.0), 100)
        : m_localVolume;

    m_remoteControlMobile = false;
    m_remotePlaying = false;
    m_sourceKind = QStringLiteral("desktop");

    if (cmdVolume != m_localVolume) {
        m_localVolume = cmdVolume;
        emit remoteVolumeCommandReceived(cmdVolume);
    }

    if (cmdTrackId > 0 && cmdTrackId != m_localTrackId) {
        m_localTrackId = cmdTrackId;
        m_localPositionSec = cmdPosSec;
        m_localPlaying = cmdPlaying;
        QJsonArray queue = state.value(QStringLiteral("active_queue")).toArray();
        if (queue.isEmpty()) queue.append(cmdTrackId);
        applyQueue(QStringLiteral("default"), queue, cmdTrackId, cmdPosSec,
                   cmdPlaying, generation, cmdPlaying);
    } else {
        if (qAbs(cmdPosSec - m_localPositionSec) >= 2) {
            m_localPositionSec = cmdPosSec;
            emit remoteSeekCommandReceived(cmdPosSec);
        }
        if (cmdPlaying && !m_localPlaying) {
            m_localPlaying = true;
            emit takeoverDesktopRequested();
        } else if (!cmdPlaying && m_localPlaying) {
            m_localPlaying = false;
            emit pauseLocalRequested();
        }
    }

    syncLocalPlayback(m_localTrackId, m_localPositionSec,
                      m_localPlaying, m_localVolume);
    emit sessionChanged();
}

void SessionPlaybackController::handleStates(quint64 generation) {
    if (m_okCount == 0) return;

    QJsonObject defaultState;
    QJsonObject mobileState;
    for (const auto &candidate : m_candidates) {
        if (candidate.first == QLatin1String("default"))
            defaultState = candidate.second;
        else if (candidate.first == QLatin1String("mobile_client"))
            mobileState = candidate.second;
    }

    const QString defaultDev =
        defaultState.value(QStringLiteral("device_id")).toString();
    if (defaultDev == QLatin1String("remote_cmd_to_desktop") ||
        defaultDev == QLatin1String("transfer_to_desktop")) {
        handleRemoteCommandToDesktop(defaultState, generation);
        return;
    }
    if (defaultDev == QLatin1String("mobile_client") && m_localPlaying) {
        emit pauseLocalRequested();
        m_localPlaying = false;
        m_remoteControlMobile = true;
        m_sourceKind = QStringLiteral("mobile");
    }

    if (m_localPlaying) {
        const bool mobilePlaying =
            mobileState.value(QStringLiteral("is_playing")).toBool();
        const QString mobileDev =
            mobileState.value(QStringLiteral("device_id")).toString();
        const double mobileAt =
            mobileState.value(QStringLiteral("updated_at")).toDouble();
        if (mobilePlaying && mobileDev == QLatin1String("mobile_client") &&
            mobileAt > m_lastLocalPlayAt + 1.5) {
            emit pauseLocalRequested();
            m_localPlaying = false;
            m_remoteControlMobile = true;
        } else {
            return;
        }
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (nowMs - m_lastRemoteCmdSentMs < 1800) {
        return;
    }

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
    const int posSec =
        qRound(state.value(QStringLiteral("position_sec")).toDouble());
    const bool playing = state.value(QStringLiteral("is_playing")).toBool();
    const QJsonValue volVal = state.value(QStringLiteral("volume"));
    const int volPct = (!volVal.isUndefined() && !volVal.isNull())
        ? qBound(0, qRound(volVal.toDouble() * 100.0), 100)
        : -1;

    m_localTrackId = trackId > 0 ? trackId : m_localTrackId;
    m_localPositionSec = qMax(0, posSec);
    if (volPct >= 0) m_localVolume = volPct;
    if (sourceKindFor(userId) == QLatin1String("mobile") && playing) {
        m_remoteControlMobile = true;
    }

    emit remoteProgressUpdated(m_localPositionSec, volPct, playing);
    applyQueue(userId, queue, trackId, m_localPositionSec, playing, generation);
}

void SessionPlaybackController::applyQueue(const QString &userId,
                                           const QJsonArray &queue,
                                           int trackId, int positionSec,
                                           bool playing, quint64 generation,
                                           bool autoPlayDesktop) {
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
    if (signature == m_lastSignature && !autoPlayDesktop) {
        updateSession(true, sourceKindFor(userId), playing, ids.size());
        return;
    }

    m_resolving = true;
    m_api->resolveTracks(ids,
        [this, generation, signature, userId, trackId, positionSec, playing,
         ids, autoPlayDesktop](bool ok, const QJsonArray &tracks) {
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
            if (autoPlayDesktop) {
                m_localPlaying = true;
                m_remotePlaying = false;
                m_sourceKind = QStringLiteral("desktop");
                emit takeoverDesktopRequested();
                emit sessionChanged();
            } else {
                updateSession(true, sourceKindFor(userId), playing,
                              resolved.size());
            }
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


void SessionPlaybackController::remoteNext() {
    if (!m_queue) return;
    const QVariantList tracks = m_queue->queueTracks();
    if (tracks.isEmpty()) return;
    int curIdx = m_queue->currentIndex();
    int nextIdx = (curIdx + 1) % tracks.size();
    m_queue->playAtIndex(nextIdx);
    int nextId = tracks[nextIdx].toMap().value(QStringLiteral("id")).toInt();
    if (nextId > 0) {
        sendRemoteCommandToAndroid(nextId, 0, true, m_localVolume);
    }
}

void SessionPlaybackController::remotePrevious() {
    if (!m_queue) return;
    const QVariantList tracks = m_queue->queueTracks();
    if (tracks.isEmpty()) return;
    int curIdx = m_queue->currentIndex();
    int prevIdx = (curIdx - 1 + tracks.size()) % tracks.size();
    m_queue->playAtIndex(prevIdx);
    int prevId = tracks[prevIdx].toMap().value(QStringLiteral("id")).toInt();
    if (prevId > 0) {
        sendRemoteCommandToAndroid(prevId, 0, true, m_localVolume);
    }
}

void SessionPlaybackController::remoteSeek(int posSec) {
    m_localPositionSec = qMax(0, posSec);
    sendRemoteCommandToAndroid(m_localTrackId, m_localPositionSec,
                               m_remotePlaying, m_localVolume);
    emit remoteProgressUpdated(m_localPositionSec, m_localVolume, m_remotePlaying);
}

void SessionPlaybackController::remoteSetVolume(int volumePct) {
    m_localVolume = qBound(0, volumePct, 100);
    sendRemoteCommandToAndroid(m_localTrackId, m_localPositionSec,
                               m_remotePlaying, m_localVolume);
    emit remoteProgressUpdated(m_localPositionSec, m_localVolume, m_remotePlaying);
}

} // namespace Dopamine::Features::Interconnect::App


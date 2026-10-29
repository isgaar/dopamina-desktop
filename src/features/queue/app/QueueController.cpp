/**
 * Location: src/features/queue/app/QueueController.cpp
 * Function: Implementation of player queue controller orchestrating dynamic
 * track lists, repeat/shuffle algorithms, and song mix queues.
 * All Rights Reserved isgaar
 */
#include "QueueController.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QJsonObject>
#include <algorithm>

namespace Dopamine::Features::Queue::App {

namespace {

QString cleanTitle(const QString &title) {
    QString t = title.trimmed();
    static const QRegularExpression verRegex(
        QStringLiteral("(?i)\\s*[\\[\\(](?:remaster(?:ed)?|original(?:\\s+version)?|radio\\s*edit|extended(?:\\s*mix)?|album\\s*version|single\\s*version|mix|edit|live|deluxe|version|mono|stereo|re-recorded|bonus\\s*track|\\d{4}\\s*remaster).*?[\\]\\)]"),
        QRegularExpression::CaseInsensitiveOption);
    t.remove(verRegex);
    static const QRegularExpression featRegex(
        QStringLiteral("(?i)\\s*[\\[\\(](?:feat\\.?|featuring|ft\\.?)\\s+[^\\)\\]]+[\\]\\)]"),
        QRegularExpression::CaseInsensitiveOption);
    t.remove(featRegex);
    static const QRegularExpression dashVerRegex(
        QStringLiteral("(?i)\\s*-\\s*(?:remaster(?:ed)?|live|edit|radio\\s*edit|mono|stereo).*$"),
        QRegularExpression::CaseInsensitiveOption);
    t.remove(dashVerRegex);
    return t.trimmed();
}

QString normalizeArtistName(const QString &artist) {
    QString a = artist.trimmed();
    static const QRegularExpression prefixRegex(
        QStringLiteral("(?i)^(?:the|los|las|el|la)\\s+"),
        QRegularExpression::CaseInsensitiveOption);
    a.remove(prefixRegex);
    return a.trimmed();
}

QString trackSignature(const QString &title, const QString &artist) {
    QString cTitle = cleanTitle(title).toLower();
    cTitle.remove(QRegularExpression(QStringLiteral("[^a-z0-9]")));
    QString cArtist = normalizeArtistName(artist).toLower();
    cArtist.remove(QRegularExpression(QStringLiteral("[^a-z0-9]")));
    return cTitle + QLatin1Char('|') + cArtist;
}

} // namespace

QueueController::QueueController(Infra::QueueApiClient *apiClient,
                                 QObject *parent)
    : QObject(parent), m_api(apiClient) {}

void QueueController::playNow(const QVariantMap &track) {
    recordListenFeedback();
    m_queue.insert(m_currentIndex + 1, track);
    m_currentIndex++;
    emit queueChanged(); emit indexChanged();
    playCurrentTrack();
}

void QueueController::playFromLibrary(const QVariantList &tracks, int index) {
    if (tracks.isEmpty()) return;
    recordListenFeedback();
    m_queue = tracks;
    m_currentIndex = qBound(0, index, tracks.size() - 1);
    m_isMixActive = false;
    emit queueChanged();
    emit indexChanged();
    playCurrentTrack();
}

void QueueController::playAtIndex(int index) {
    if (index < 0 || index >= m_queue.size()) return;
    recordListenFeedback();
    m_currentIndex = index;
    emit indexChanged();
    playCurrentTrack();
}

QString QueueController::nextTrackPath() const {
    if (m_queue.isEmpty()) return QString{};
    int nextIdx = m_currentIndex + 1;
    if (nextIdx >= m_queue.size()) {
        if (!m_isRepeat) return QString{};
        nextIdx = 0;
    }
    const QVariantMap nextTrack = m_queue[nextIdx].toMap();
    return nextTrack.value(QStringLiteral("path")).toString();
}

void QueueController::playNext(const QVariantMap &track) {
    if (m_queue.isEmpty() || m_currentIndex < 0) {
        recordListenFeedback();
        m_queue.append(track);
        m_currentIndex = 0;
        emit queueChanged();
        emit indexChanged();
        playCurrentTrack();
        return;
    }
    const int pos = qBound(0, m_currentIndex + 1, m_queue.size());
    m_queue.insert(pos, track);
    emit queueChanged();
}

void QueueController::appendToQueue(const QVariantMap &track) {
    m_queue.append(track);
    emit queueChanged();
}

void QueueController::clearQueue() {
    recordListenFeedback();
    m_queue.clear(); m_currentIndex = -1;
    emit queueChanged(); emit indexChanged();
}

void QueueController::applySessionQueue(const QVariantList &tracks,
                                        int currentIndex, int positionSec) {
    if (tracks.isEmpty()) return;
    recordListenFeedback();
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

    m_currentTrackId = id;
    m_currentTrackDurationSec = duration;
    m_currentTrackStartTimeMs = QDateTime::currentMSecsSinceEpoch();

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

    recordListenFeedback();

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
        recordListenFeedback();
        m_currentIndex--;
        emit indexChanged();
        playCurrentTrack();
    }
}

void QueueController::startMix(int seedTrackId, const QString &seedTitle) {
    QVariantMap seedTrack;
    seedTrack[QStringLiteral("id")] = seedTrackId;
    seedTrack[QStringLiteral("title")] = seedTitle;
    startMixTrack(seedTrack);
}

void QueueController::startMixTrack(const QVariantMap &seedTrack) {
    recordListenFeedback();
    const int seedTrackId = seedTrack.value(QStringLiteral("id")).toInt();
    QString seedTitle = seedTrack.value(QStringLiteral("title")).toString();
    if (seedTitle.isEmpty())
        seedTitle = seedTrack.value(QStringLiteral("titulo")).toString();
    m_isMixActive = true;
    m_seedId = seedTrackId;
    m_mixSeedTitle = seedTitle;
    emit mixChanged();
    m_queue.clear();
    m_currentIndex = 0;
    m_queue.append(seedTrack);
    emit queueChanged();
    emit indexChanged();
    playCurrentTrack();
    refillMixIfNeeded();
}

void QueueController::startRadioTrack(const QVariantMap &seedTrack) {
    startMixTrack(seedTrack);
}

void QueueController::refillMixIfNeeded() {
    if (!m_api || !m_isMixActive || m_isRefillingMix) return;
    int remaining = m_queue.size() - m_currentIndex;
    if (remaining >= 5) return;

    m_isRefillingMix = true;

    QSet<int> seenIds;
    QSet<QString> seenSignatures;

    for (const auto &itemVal : m_queue) {
        const QVariantMap item = itemVal.toMap();
        const int id = item.value(QStringLiteral("id")).toInt();
        if (id > 0) seenIds.insert(id);

        QString t = item.value(QStringLiteral("title")).toString();
        if (t.isEmpty()) t = item.value(QStringLiteral("titulo")).toString();
        QString a = item.value(QStringLiteral("artist")).toString();
        if (a.isEmpty()) a = item.value(QStringLiteral("artista")).toString();
        QString sig = trackSignature(t, a);
        if (!sig.isEmpty() && sig != QLatin1String("|")) {
            seenSignatures.insert(sig);
        }
    }

    m_api->fetchMixNext(m_seedId, m_history, {}, QStringLiteral("normal"), 25,
        [this, seenIds, seenSignatures](bool ok, const QJsonArray &ids, const QJsonArray &tracks) mutable {
            m_isRefillingMix = false;
            if (!ok) return;

            QSettings affinitySettings(QStringLiteral("DopamineFM"), QStringLiteral("MixAffinity"));
            QList<QVariantMap> candidates;

            if (!tracks.isEmpty()) {
                for (const auto &val : tracks) {
                    QJsonObject obj = val.toObject();
                    int id = obj.value(QStringLiteral("id")).toInt();
                    if (id <= 0) id = obj.value(QStringLiteral("cancion_id")).toInt();
                    if (id <= 0 || seenIds.contains(id)) continue;

                    QString title = obj.value(QStringLiteral("title")).toString();
                    if (title.isEmpty()) title = obj.value(QStringLiteral("titulo")).toString();
                    QString artist = obj.value(QStringLiteral("artist")).toString();
                    if (artist.isEmpty()) artist = obj.value(QStringLiteral("artista")).toString();

                    QString sig = trackSignature(title, artist);
                    if (!sig.isEmpty() && sig != QLatin1String("|") && seenSignatures.contains(sig)) {
                        continue; // Skip duplicate version of the same song
                    }

                    seenIds.insert(id);
                    if (!sig.isEmpty() && sig != QLatin1String("|")) {
                        seenSignatures.insert(sig);
                    }

                    QVariantMap item;
                    item[QStringLiteral("id")] = id;
                    item[QStringLiteral("title")] = title.isEmpty() ? QStringLiteral("Pista #%1").arg(id) : title;
                    item[QStringLiteral("artist")] = artist;
                    item[QStringLiteral("album")] = obj.value(QStringLiteral("album")).toString();
                    int duration = obj.value(QStringLiteral("duration")).toInt();
                    if (duration <= 0) duration = obj.value(QStringLiteral("duracion")).toInt();
                    item[QStringLiteral("duration")] = duration;
                    item[QStringLiteral("path")] = obj.value(QStringLiteral("path")).toString();
                    item[QStringLiteral("isMix")] = true;

                    const QString pairKey = QStringLiteral("pair_%1_%2").arg(m_seedId).arg(id);
                    item[QStringLiteral("_affinityScore")] = affinitySettings.value(pairKey, 0.0).toDouble();

                    candidates.append(item);
                }
            } else {
                for (const auto &val : ids) {
                    int id = val.toInt();
                    if (id <= 0 || seenIds.contains(id)) continue;
                    seenIds.insert(id);

                    QVariantMap item;
                    item[QStringLiteral("id")] = id;
                    item[QStringLiteral("title")] = QStringLiteral("Pista #%1").arg(id);
                    item[QStringLiteral("isMix")] = true;

                    const QString pairKey = QStringLiteral("pair_%1_%2").arg(m_seedId).arg(id);
                    item[QStringLiteral("_affinityScore")] = affinitySettings.value(pairKey, 0.0).toDouble();
                    candidates.append(item);
                }
            }

            // Stable sort by affinity score descending so positive affinity tracks come forward
            std::stable_sort(candidates.begin(), candidates.end(),
                [](const QVariantMap &a, const QVariantMap &b) {
                    return a.value(QStringLiteral("_affinityScore")).toDouble() >
                           b.value(QStringLiteral("_affinityScore")).toDouble();
                });

            bool addedAny = false;
            for (const auto &cand : candidates) {
                m_queue.append(cand);
                addedAny = true;
            }

            if (addedAny) {
                emit queueChanged();
            }
        });
}

void QueueController::playCurrentTrack() {
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) return;
    QVariantMap t = m_queue[m_currentIndex].toMap();
    int id = t.value(QStringLiteral("id")).toInt();
    if (id <= 0) return;

    m_history.append(id);
    if (m_history.size() > 50) m_history.removeFirst();
    if (m_api) m_api->recordHistory(id);

    QString title = t.value(QStringLiteral("title")).toString();
    if (title.isEmpty()) title = t.value(QStringLiteral("titulo")).toString();
    QString artist = t.value(QStringLiteral("artist")).toString();
    if (artist.isEmpty()) artist = t.value(QStringLiteral("artista")).toString();
    int duration = t.value(QStringLiteral("duration")).toInt();
    if (duration <= 0) duration = t.value(QStringLiteral("duracion")).toInt();

    m_currentTrackId = id;
    m_currentTrackDurationSec = duration;
    m_currentTrackStartTimeMs = QDateTime::currentMSecsSinceEpoch();

    emit playTrackRequested(
        id, title, artist,
        t.value(QStringLiteral("album")).toString(),
        t.value(QStringLiteral("path")).toString(),
        duration);
}

void QueueController::recordListenFeedback() {
    if (m_currentTrackId <= 0 || m_currentTrackStartTimeMs <= 0) return;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const double playedSec = qMax(0.0, (now - m_currentTrackStartTimeMs) / 1000.0);
    const int duration = m_currentTrackDurationSec;
    const double ratio = duration > 0 ? (playedSec / duration) : 0.0;

    QString eventType;
    double affinityDelta = 0.0;

    if (ratio >= 0.60 || playedSec >= 90.0) {
        eventType = QStringLiteral("like");
        affinityDelta = 1.0;
    } else if (ratio < 0.20 && playedSec < 35.0) {
        eventType = QStringLiteral("skip");
        affinityDelta = -1.0;
    }

    if (!eventType.isEmpty() && m_seedId > 0) {
        QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("MixAffinity"));
        const QString pairKey = QStringLiteral("pair_%1_%2").arg(m_seedId).arg(m_currentTrackId);
        double currentScore = settings.value(pairKey, 0.0).toDouble();
        settings.setValue(pairKey, currentScore + affinityDelta);

        if (m_api) {
            m_api->reportMixEvent(m_seedId, m_currentTrackId, eventType, playedSec, m_history);
        }
    }

    m_currentTrackId = 0;
    m_currentTrackStartTimeMs = 0;
    m_currentTrackDurationSec = 0;
}

} // namespace Dopamine::Features::Queue::App

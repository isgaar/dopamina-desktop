/**
 * Location: src/features/interconnect/infra/SessionPlaybackApiClient.cpp
 * Function: Fetches playback state, saved sessions, and queue metadata from the
 * server interconnect API with defensive JSON parsing.
 * All Rights Reserved isgaar
 */
#include "SessionPlaybackApiClient.h"
#include <QJsonDocument>
#include <QJsonValue>
#include <QUrlQuery>

namespace Dopamine::Features::Interconnect::Infra {

SessionPlaybackApiClient::SessionPlaybackApiClient(Core::ApiClient *client,
                                                   QObject *parent)
    : QObject(parent), m_client(client) {}

void SessionPlaybackApiClient::fetchState(const QString &userId,
                                          StateCallback cb) {
    if (!m_client) { if (cb) cb(false, {}); return; }
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("user_id"), userId);
    const QString path = QStringLiteral("/api/v1/player/playback/state?") +
                         query.toString(QUrl::FullyEncoded);
    m_client->get(path, [cb](bool ok, const QByteArray &data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        cb(doc.isObject(), doc.object());
    });
}

void SessionPlaybackApiClient::fetchSavedSession(const QString &userId,
                                                 StateCallback cb) {
    if (!m_client) { if (cb) cb(false, {}); return; }
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("user_id"), userId);
    const QString path = QStringLiteral("/api/v1/player/playback/session?") +
                         query.toString(QUrl::FullyEncoded);
    m_client->get(path, [cb](bool ok, const QByteArray &data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) { cb(false, {}); return; }
        const QJsonObject envelope = doc.object();
        const QJsonValue session = envelope.value(QStringLiteral("session"));
        cb(session.isObject(), session.toObject());
    });
}

void SessionPlaybackApiClient::resolveTracks(const QList<int> &ids,
                                             TracksCallback cb) {
    if (!m_client || ids.isEmpty()) { if (cb) cb(false, {}); return; }
    QStringList tokens;
    tokens.reserve(ids.size());
    for (int id : ids) tokens.append(QString::number(id));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("ids"), tokens.join(QLatin1Char(',')));
    const QString path = QStringLiteral("/api/v1/tracks/resolve?") +
                         query.toString(QUrl::FullyEncoded);
    m_client->get(path, [cb](bool ok, const QByteArray &data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) cb(true, doc.array());
        else cb(false, {});
    });
}

void SessionPlaybackApiClient::publishState(const QJsonObject &payload) {
    if (!m_client || payload.isEmpty()) return;
    m_client->post(QStringLiteral("/api/v1/player/playback/state"),
                   QJsonDocument(payload).toJson(QJsonDocument::Compact),
                   [](bool, const QByteArray &) {});
}

void SessionPlaybackApiClient::saveSession(const QJsonObject &payload) {
    if (!m_client || payload.isEmpty()) return;
    m_client->post(QStringLiteral("/api/v1/player/playback/session"),
                   QJsonDocument(payload).toJson(QJsonDocument::Compact),
                   [](bool, const QByteArray &) {});
}

} // namespace Dopamine::Features::Interconnect::Infra

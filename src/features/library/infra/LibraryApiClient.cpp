/**
 * Location: src/features/library/infra/LibraryApiClient.cpp
 * Function: Implementation of catalog network client fetching artists, albums,
 * and track collections with query parameter formatting and JSON parsing.
 * All Rights Reserved isgaar
 */
#include "LibraryApiClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace Dopamine::Features::Library::Infra {

LibraryApiClient::LibraryApiClient(Core::ApiClient *client, QObject *parent)
    : QObject(parent), m_client(client) {}

void LibraryApiClient::fetchTracks(int page, int limit, const QString &query,
                                   CatalogCallback cb) {
    if (!m_client) { if (cb) cb(false, {}); return; }
    const int offset = qMax(0, page - 1) * limit;
    QUrlQuery q;
    q.addQueryItem("limit", QString::number(limit));
    q.addQueryItem("offset", QString::number(offset));
    if (!query.isEmpty()) q.addQueryItem("q", query);

    QString path = "/api/v1/tracks?" + q.toString(QUrl::FullyEncoded);
    m_client->get(path, [cb](bool ok, const QByteArray &data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            QJsonArray normalized;
            for (const auto &value : doc.array()) {
                QJsonObject track = value.toObject();
                QString stream =
                    track.value("streamUrl").toString();
                if (stream.isEmpty())
                    stream = track.value("stream_url").toString();
                if (!stream.isEmpty())
                    track.insert("path", stream);
                normalized.append(track);
            }
            cb(true, normalized);
        } else {
            cb(false, {});
        }
    });
}

void LibraryApiClient::fetchAlbums(int page, int limit, CatalogCallback cb) {
    if (!m_client) { if (cb) cb(false, {}); return; }
    QString path = QString("/api/v1/catalog/albums?page=%1&limit=%2")
                       .arg(page).arg(limit);
    m_client->get(path, [cb](bool ok, const QByteArray &data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            cb(true, doc.object().value("items").toArray());
        } else if (doc.isArray()) {
            cb(true, doc.array());
        } else {
            cb(false, {});
        }
    });
}

void LibraryApiClient::fetchArtists(int page, int limit, CatalogCallback cb) {
    if (!m_client) { if (cb) cb(false, {}); return; }
    QString path = QString("/api/v1/catalog/artists?page=%1&limit=%2")
                       .arg(page).arg(limit);
    m_client->get(path, [cb](bool ok, const QByteArray &data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            cb(true, doc.object().value("items").toArray());
        } else if (doc.isArray()) {
            cb(true, doc.array());
        } else {
            cb(false, {});
        }
    });
}

} // namespace Dopamine::Features::Library::Infra

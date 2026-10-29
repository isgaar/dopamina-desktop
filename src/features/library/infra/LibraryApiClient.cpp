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
        QJsonArray sourceArray;
        if (doc.isArray()) {
            sourceArray = doc.array();
        } else if (doc.isObject()) {
            sourceArray = doc.object().value("items").toArray();
        } else {
            cb(false, {});
            return;
        }
        QJsonArray normalized;
        for (const auto &value : sourceArray) {
            QJsonObject track = value.toObject();
            const int id = track.value("id").toInt();
            QString stream = track.value("streamUrl").toString();
            if (stream.isEmpty())
                stream = track.value("stream_url").toString();
            if (!stream.isEmpty())
                track.insert("path", stream);
            QString cover = track.value("coverUrl").toString();
            if (cover.isEmpty())
                cover = track.value("cover_url").toString();
            if (cover.isEmpty() && id > 0 && stream.contains("/api/v1/")) {
                const int idx = stream.indexOf("/api/v1/");
                cover = stream.left(idx) +
                        QString("/api/v1/covers/%1").arg(id);
            }
            if (!cover.isEmpty()) {
                track.insert("coverUrl", cover);
                track.insert("cover", cover);
            }
            normalized.append(track);
        }
        cb(true, normalized);
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

void LibraryApiClient::fetchTrackMetadata(int trackId, ObjectCallback cb) {
    if (!m_client || trackId <= 0) { if (cb) cb(false, {}); return; }
    QString path = QString("/api/v1/tracks/%1/metadata").arg(trackId);
    m_client->get(path, [this, trackId, cb](bool ok, const QByteArray &data) {
        if (ok) {
            auto doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                if (cb) cb(true, doc.object());
                return;
            }
        }
        QString fallback = QString("/api/v1/tracks/%1/acoustic").arg(trackId);
        m_client->get(fallback, [cb](bool ok2, const QByteArray &data2) {
            if (!ok2 || !cb) { if (cb) cb(false, {}); return; }
            auto doc2 = QJsonDocument::fromJson(data2);
            cb(doc2.isObject(), doc2.isObject() ? doc2.object() : QJsonObject());
        });
    });
}

void LibraryApiClient::downloadTrackStream(int trackId, BytesCallback cb) {
    if (!m_client || trackId <= 0) { if (cb) cb(false, {}); return; }
    QString path = QString("/api/v1/streams/%1").arg(trackId);
    m_client->get(path, [cb](bool ok, const QByteArray &data) {
        if (cb) cb(ok && !data.isEmpty(), data);
    });
}

} // namespace Dopamine::Features::Library::Infra

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
    QUrlQuery q;
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("limit", QString::number(limit));
    if (!query.isEmpty()) q.addQueryItem("q", query);

    QString path = "/api/v1/catalog/tracks?" + q.toString(QUrl::FullyEncoded);
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

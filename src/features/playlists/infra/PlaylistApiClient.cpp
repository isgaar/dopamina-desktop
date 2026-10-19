/**
 * Location: src/features/playlists/infra/PlaylistApiClient.cpp
 * Function: Implementation of playlist network client invoking playlist CRUD
 * operations and favorite track likes on the backend server.
 * All Rights Reserved isgaar
 */
#include "PlaylistApiClient.h"
#include <QJsonObject>
#include <QJsonDocument>

namespace Dopamine::Features::Playlists::Infra {

PlaylistApiClient::PlaylistApiClient(Core::ApiClient *client, QObject *parent)
    : QObject(parent), m_client(client) {}

void PlaylistApiClient::fetchPlaylists(ListCallback cb) {
    if (!m_client) { if (cb) cb(false, {}); return; }
    m_client->get("/api/v1/playlists", [cb](bool ok, auto data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(data);
        cb(true, doc.isArray() ? doc.array() : QJsonArray());
    });
}

void PlaylistApiClient::createPlaylist(const QString &name,
                                       StatusCallback cb) {
    if (!m_client) { if (cb) cb(false); return; }
    QJsonObject obj; obj["nombre"] = name;
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    m_client->post("/api/v1/playlists", body, [cb](bool ok, auto) {
        if (cb) cb(ok);
    });
}

void PlaylistApiClient::deletePlaylist(int id, StatusCallback cb) {
    if (!m_client) { if (cb) cb(false); return; }
    QString path = QString("/api/v1/playlists/%1").arg(id);
    m_client->deleteResource(path, [cb](bool ok, auto) {
        if (cb) cb(ok);
    });
}

void PlaylistApiClient::fetchLikes(ListCallback cb) {
    if (!m_client) { if (cb) cb(false, {}); return; }
    m_client->get("/api/v1/likes", [cb](bool ok, auto data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(data);
        cb(true, doc.isArray() ? doc.array() : QJsonArray());
    });
}

void PlaylistApiClient::setLike(int trackId, bool liked, StatusCallback cb) {
    if (!m_client || trackId <= 0) { if (cb) cb(false); return; }
    QString path = QString("/api/v1/likes/%1").arg(trackId);
    if (liked) {
        m_client->put(path, "{}", [cb](bool ok, auto) { if (cb) cb(ok); });
    } else {
        m_client->deleteResource(path, [cb](bool ok, auto) {
            if (cb) cb(ok);
        });
    }
}

} // namespace Dopamine::Features::Playlists::Infra

#include <QSettings>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
/**
 * Location: src/features/playlists/app/PlaylistController.cpp
 * Function: Implementation of playlist controller populating user collections,
 * reacting to playlist modifications, and syncing track likes.
 * All Rights Reserved isgaar
 */
#include "PlaylistController.h"
#include "core/ProjectConfig.h"
#include <QJsonObject>

namespace Dopamine::Features::Playlists::App {

PlaylistController::PlaylistController(Infra::PlaylistApiClient *apiClient,
                                       QObject *parent)
    : QObject(parent), m_api(apiClient) {
    if (Core::ProjectConfig::instance().sourceType() ==
        QStringLiteral("server")) {
        loadPlaylists(); loadLikes();
    }
}

void PlaylistController::loadPlaylists() {
    if (!m_api) {
        loadCachedPlaylists();
        return;
    }
    m_isLoading = true; emit loadingChanged();
    m_api->fetchPlaylists([this](bool ok, auto arr) {
        m_isLoading = false; emit loadingChanged();
        if (ok) {
            m_isServerOnline = true;
            emit serverOnlineChanged();
            m_playlists.clear();
            QJsonArray cachedArr;
            for (const auto &val : arr) {
                QJsonObject obj = val.toObject();
                m_playlists.append(obj.toVariantMap());
                cachedArr.append(obj);
            }
            QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("Playlists"));
            settings.setValue(QStringLiteral("cached_json"), QJsonDocument(cachedArr).toJson(QJsonDocument::Compact));
            emit playlistsChanged();
        } else {
            m_isServerOnline = false;
            emit serverOnlineChanged();
            loadCachedPlaylists();
        }
    });
}

void PlaylistController::loadCachedPlaylists() {
    QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("Playlists"));
    QByteArray json = settings.value(QStringLiteral("cached_json")).toByteArray();
    if (!json.isEmpty()) {
        auto doc = QJsonDocument::fromJson(json);
        if (doc.isArray()) {
            m_playlists.clear();
            for (const auto &val : doc.array()) {
                m_playlists.append(val.toObject().toVariantMap());
            }
            emit playlistsChanged();
        }
    }
}

QVariantList PlaylistController::effectivePlaylists() const {
    if (m_isServerOnline) return m_playlists;
    QVariantList filtered;
    for (const QVariant &item : m_playlists) {
        QVariantMap map = item.toMap();
        int total = map.value(QStringLiteral("total_pistas")).toInt();
        QVariantList tracks = map.value(QStringLiteral("pistas")).toList();
        bool hasDownloaded = false;
        if (!tracks.isEmpty()) {
            for (const QVariant &t : tracks) {
                int tid = t.toMap().value(QStringLiteral("id")).toInt();
                if (isTrackDownloaded(tid)) {
                    hasDownloaded = true;
                    break;
                }
            }
        } else if (total > 0) {
            QSettings dlSettings(QStringLiteral("DopamineFM"), QStringLiteral("Downloads"));
            if (!dlSettings.allKeys().isEmpty()) hasDownloaded = true;
        }
        if (hasDownloaded) filtered.append(map);
    }
    return filtered;
}

bool PlaylistController::isTrackDownloaded(int trackId) const {
    if (trackId <= 0) return false;
    QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("Downloads"));
    QString key = QStringLiteral("track_%1").arg(trackId);
    if (settings.contains(key)) {
        QString path = settings.value(key).toString();
        return (!path.isEmpty() && QFile::exists(path));
    }
    return false;
}

void PlaylistController::loadLikes() {
    if (!m_api) {
        loadCachedLikes();
        return;
    }
    m_api->fetchLikes([this](bool ok, auto arr) {
        if (ok) {
            m_isServerOnline = true;
            emit serverOnlineChanged();
            m_likedSet.clear();
            QJsonArray cachedArr;
            for (const auto &val : arr) {
                QJsonObject obj = val.toObject();
                int tid = obj.value(QStringLiteral("cancion_id")).toInt();
                if (tid <= 0) tid = obj.value(QStringLiteral("id")).toInt();
                if (tid > 0) {
                    m_likedSet.insert(tid);
                    cachedArr.append(obj);
                }
            }
            QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("Likes"));
            settings.setValue(QStringLiteral("cached_json"), QJsonDocument(cachedArr).toJson(QJsonDocument::Compact));
            emit likesChanged();
        } else {
            m_isServerOnline = false;
            emit serverOnlineChanged();
            loadCachedLikes();
        }
    });
}

void PlaylistController::loadCachedLikes() {
    QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("Likes"));
    QByteArray json = settings.value(QStringLiteral("cached_json")).toByteArray();
    if (!json.isEmpty()) {
        auto doc = QJsonDocument::fromJson(json);
        if (doc.isArray()) {
            m_likedSet.clear();
            for (const auto &val : doc.array()) {
                QJsonObject obj = val.toObject();
                int tid = obj.value(QStringLiteral("cancion_id")).toInt();
                if (tid <= 0) tid = obj.value(QStringLiteral("id")).toInt();
                if (tid > 0) m_likedSet.insert(tid);
            }
            emit likesChanged();
        }
    }
}

void PlaylistController::createPlaylist(const QString &name) {
    if (!m_api || name.trimmed().isEmpty()) return;
    m_api->createPlaylist(name.trimmed(), [this](bool ok) {
        if (ok) loadPlaylists();
    });
}

void PlaylistController::deletePlaylist(int id) {
    if (!m_api || id <= 0) return;
    m_api->deletePlaylist(id, [this](bool ok) {
        if (ok) loadPlaylists();
    });
}

void PlaylistController::addTrackToPlaylist(int playlistId, int trackId) {
    if (!m_api || playlistId <= 0 || trackId <= 0) return;
    m_api->addTrackToPlaylist(playlistId, trackId, [this](bool ok) {
        if (ok) loadPlaylists();
    });
}

void PlaylistController::toggleLike(int trackId) {
    if (trackId == 0) return;
    bool currentlyLiked = m_likedSet.contains(trackId);
    bool nextState = !currentlyLiked;
    if (nextState) m_likedSet.insert(trackId);
    else m_likedSet.remove(trackId);
    emit likesChanged();
    if (!m_api || trackId <= 0 ||
        Core::ProjectConfig::instance().sourceType() != QStringLiteral("server")) {
        return;
    }
    m_api->setLike(trackId, nextState,
        [this, trackId, currentlyLiked](bool ok) {
        if (!ok) {
            if (currentlyLiked) m_likedSet.insert(trackId);
            else m_likedSet.remove(trackId);
            emit likesChanged();
        }
    });
}

} // namespace Dopamine::Features::Playlists::App

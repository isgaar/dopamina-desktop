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
    if (!m_api) return;
    m_isLoading = true; emit loadingChanged();
    m_api->fetchPlaylists([this](bool ok, auto arr) {
        m_isLoading = false; emit loadingChanged();
        if (!ok) return;
        m_playlists.clear();
        for (const auto &val : arr) {
            m_playlists.append(val.toObject().toVariantMap());
        }
        emit playlistsChanged();
    });
}

void PlaylistController::loadLikes() {
    if (!m_api) return;
    m_api->fetchLikes([this](bool ok, auto arr) {
        if (!ok) return;
        m_likedSet.clear();
        for (const auto &val : arr) {
            m_likedSet.insert(val.toObject().value("cancion_id").toInt());
        }
        emit likesChanged();
    });
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

void PlaylistController::toggleLike(int trackId) {
    if (!m_api || trackId <= 0) return;
    bool currentlyLiked = m_likedSet.contains(trackId);
    bool nextState = !currentlyLiked;
    if (nextState) m_likedSet.insert(trackId);
    else m_likedSet.remove(trackId);
    emit likesChanged();
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

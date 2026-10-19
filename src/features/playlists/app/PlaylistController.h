/**
 * Location: src/features/playlists/app/PlaylistController.h
 * Function: Playlists coordinator managing user playlist lifecycle, track
 * collections, and favorite song likes persisted on the server.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYLISTS_APP_PLAYLISTCONTROLLER_H
#define DOPAMINE_FEATURES_PLAYLISTS_APP_PLAYLISTCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QSet>
#include "../infra/PlaylistApiClient.h"

namespace Dopamine::Features::Playlists::App {

class PlaylistController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList playlists READ playlists NOTIFY playlistsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)

public:
    explicit PlaylistController(Infra::PlaylistApiClient *apiClient,
                              QObject *parent = nullptr);

    QVariantList playlists() const { return m_playlists; }
    bool isLoading() const { return m_isLoading; }

    Q_INVOKABLE void loadPlaylists();
    Q_INVOKABLE void createPlaylist(const QString &name);
    Q_INVOKABLE void deletePlaylist(int id);
    Q_INVOKABLE void toggleLike(int trackId);
    Q_INVOKABLE bool isLiked(int trackId) const {
        return m_likedSet.contains(trackId);
    }

signals:
    void playlistsChanged();
    void likesChanged();
    void loadingChanged();

private:
    void loadLikes();

    Infra::PlaylistApiClient *m_api = nullptr;
    QVariantList m_playlists;
    QSet<int> m_likedSet;
    bool m_isLoading = false;
};

} // namespace Dopamine::Features::Playlists::App

#endif // DOPAMINE_FEATURES_PLAYLISTS_APP_PLAYLISTCONTROLLER_H

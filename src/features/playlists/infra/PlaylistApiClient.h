/**
 * Location: src/features/playlists/infra/PlaylistApiClient.h
 * Function: Network client for playlist creation, track list retrieval, and
 * favorite track liking operations dispatching to server player routes.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYLISTS_INFRA_PLAYLISTAPICLIENT_H
#define DOPAMINE_FEATURES_PLAYLISTS_INFRA_PLAYLISTAPICLIENT_H

#include <QObject>
#include <QJsonArray>
#include <functional>
#include "core/ApiClient.h"

namespace Dopamine::Features::Playlists::Infra {

class PlaylistApiClient : public QObject {
    Q_OBJECT
public:
    explicit PlaylistApiClient(Core::ApiClient *client,
                               QObject *parent = nullptr);

    using ListCallback = std::function<void(bool ok, const QJsonArray &arr)>;
    using StatusCallback = std::function<void(bool ok)>;

    void fetchPlaylists(ListCallback cb);
    void createPlaylist(const QString &name, StatusCallback cb);
    void deletePlaylist(int id, StatusCallback cb);
    void addTrackToPlaylist(int playlistId, int trackId, StatusCallback cb);
    void fetchLikes(ListCallback cb);
    void setLike(int trackId, bool liked, StatusCallback cb);

private:
    Core::ApiClient *m_client = nullptr;
};

} // namespace Dopamine::Features::Playlists::Infra

#endif // DOPAMINE_FEATURES_PLAYLISTS_INFRA_PLAYLISTAPICLIENT_H

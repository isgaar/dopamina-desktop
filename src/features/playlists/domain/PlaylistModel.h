/**
 * Location: src/features/playlists/domain/PlaylistModel.h
 * Function: Data structure defining playlist metadata and track association
 * records for synchronization with the server database.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYLISTS_DOMAIN_PLAYLISTMODEL_H
#define DOPAMINE_FEATURES_PLAYLISTS_DOMAIN_PLAYLISTMODEL_H

#include <QString>
#include <QVariantMap>

namespace Dopamine::Features::Playlists::Domain {

struct PlaylistItem {
    int id = 0;
    QString name;
    int trackCount = 0;

    QVariantMap toVariantMap() const {
        return {
            {"id", id},
            {"nombre", name},
            {"canciones", trackCount}
        };
    }
};

} // namespace Dopamine::Features::Playlists::Domain

#endif // DOPAMINE_FEATURES_PLAYLISTS_DOMAIN_PLAYLISTMODEL_H

/**
 * Location: src/features/library/infra/LibraryApiClient.h
 * Function: Network adapter dispatching HTTP catalog browse, paging, and search
 * requests to server endpoints with bearer authentication tokens.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_LIBRARY_INFRA_LIBRARYAPICLIENT_H
#define DOPAMINE_FEATURES_LIBRARY_INFRA_LIBRARYAPICLIENT_H

#include <QObject>
#include <QJsonArray>
#include <functional>
#include "core/ApiClient.h"

namespace Dopamine::Features::Library::Infra {

class LibraryApiClient : public QObject {
    Q_OBJECT
public:
    explicit LibraryApiClient(Core::ApiClient *client,
                              QObject *parent = nullptr);

    using CatalogCallback =
        std::function<void(bool ok, const QJsonArray &data)>;

    void fetchTracks(int page, int limit, const QString &query,
                     CatalogCallback cb);
    void fetchAlbums(int page, int limit, CatalogCallback cb);
    void fetchArtists(int page, int limit, CatalogCallback cb);

private:
    Core::ApiClient *m_client = nullptr;
};

} // namespace Dopamine::Features::Library::Infra

#endif // DOPAMINE_FEATURES_LIBRARY_INFRA_LIBRARYAPICLIENT_H

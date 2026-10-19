/**
 * Location: src/features/library/app/LibraryController.h
 * Function: Application controller managing library views, artist catalogs,
 * recursive directory discovery, track paging, and search filter dispatching.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_LIBRARY_APP_LIBRARYCONTROLLER_H
#define DOPAMINE_FEATURES_LIBRARY_APP_LIBRARYCONTROLLER_H

#include <QObject>
#include <QSet>
#include <QVariantList>
#include "../infra/LibraryApiClient.h"
#include "../infra/RecursiveDirectoryScanner.h"

namespace Dopamine::Features::Library::App {

class LibraryController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList albums READ albums NOTIFY albumsChanged)
    Q_PROPERTY(QVariantList artists READ artists NOTIFY artistsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery
               NOTIFY searchQueryChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage
               NOTIFY statusMessageChanged)
    Q_PROPERTY(int totalTracks READ totalTracks NOTIFY tracksChanged)
    Q_PROPERTY(double totalDurationHours READ totalDurationHours
               NOTIFY tracksChanged)

public:
    explicit LibraryController(Infra::LibraryApiClient *apiClient,
                               QObject *parent = nullptr);

    QVariantList tracks() const { return m_tracks; }
    QVariantList albums() const { return m_albums; }
    QVariantList artists() const { return m_artists; }
    bool isLoading() const { return m_isLoading; }
    QString searchQuery() const { return m_searchQuery; }
    QString statusMessage() const { return m_statusMessage; }
    int totalTracks() const { return m_tracks.size(); }
    double totalDurationHours() const;

    void setSearchQuery(const QString &q);

    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void loadTracks(int page = 1);
    Q_INVOKABLE void loadAlbums(int page = 1);
    Q_INVOKABLE void loadArtists(int page = 1);

signals:
    void tracksChanged();
    void albumsChanged();
    void artistsChanged();
    void loadingChanged();
    void searchQueryChanged();
    void statusMessageChanged();

private:
    void handleDiscoveredBatch(const QVariantList &batch);
    void applySearchFilter();

    Infra::LibraryApiClient *m_api = nullptr;
    Infra::RecursiveDirectoryScanner *m_scanner = nullptr;
    QVariantList m_allTracks;
    QVariantList m_tracks;
    QVariantList m_albums;
    QVariantList m_artists;
    QSet<QString> m_knownAlbums;
    QSet<QString> m_knownArtists;
    QString m_searchQuery;
    QString m_statusMessage;
    bool m_isLoading = false;
};

} // namespace Dopamine::Features::Library::App

#endif // DOPAMINE_FEATURES_LIBRARY_APP_LIBRARYCONTROLLER_H

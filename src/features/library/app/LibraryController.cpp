/**
 * Location: src/features/library/app/LibraryController.cpp
 * Function: Implementation of library controller binding catalog API responses
 * and real-time recursive directory scanner batches to reactive QML models.
 * All Rights Reserved isgaar
 */
#include "LibraryController.h"
#include <QJsonObject>

namespace Dopamine::Features::Library::App {

LibraryController::LibraryController(Infra::LibraryApiClient *apiClient,
                                     QObject *parent)
    : QObject(parent), m_api(apiClient),
      m_scanner(new Infra::RecursiveDirectoryScanner(this)) {
    connect(m_scanner, &Infra::RecursiveDirectoryScanner::batchDiscovered,
            this, &LibraryController::handleDiscoveredBatch);
    connect(m_scanner, &Infra::RecursiveDirectoryScanner::scanFinished,
            this, [this](int total) {
                m_isLoading = false;
                m_statusMessage = QStringLiteral("%1 canciones").arg(total);
                emit loadingChanged(); emit statusMessageChanged();
            });
}
void LibraryController::handleDiscoveredBatch(const QVariantList &batch) {
    for (const auto &item : batch) {
        m_allTracks.append(item);
        auto m = item.toMap();
        QString alb = m.value(QStringLiteral("album")).toString();
        if (!alb.isEmpty() && !m_knownAlbums.contains(alb)) {
            m_knownAlbums.insert(alb);
            m_albums.append(QVariantMap{{QStringLiteral("name"), alb}});
        }
        QString art = m.value(QStringLiteral("artist")).toString();
        if (!art.isEmpty() && !m_knownArtists.contains(art)) {
            m_knownArtists.insert(art);
            m_artists.append(QVariantMap{{QStringLiteral("name"), art}});
        }
    }
    applySearchFilter();
    m_statusMessage = QStringLiteral("%1 canciones").arg(m_allTracks.size());
    emit statusMessageChanged(); emit albumsChanged(); emit artistsChanged();
}
void LibraryController::applySearchFilter() {
    if (m_searchQuery.isEmpty()) {
        m_tracks = m_allTracks;
    } else {
        m_tracks.clear();
        QString q = m_searchQuery.toLower();
        for (const auto &v : m_allTracks) {
            auto m = v.toMap();
            bool t = m.value(QStringLiteral("titulo"))
                         .toString().toLower().contains(q);
            bool a = m.value(QStringLiteral("artista"))
                         .toString().toLower().contains(q);
            bool b = m.value(QStringLiteral("album"))
                         .toString().toLower().contains(q);
            if (t || a || b) m_tracks.append(v);
        }
    }
    emit tracksChanged();
}
void LibraryController::setSearchQuery(const QString &q) {
    if (m_searchQuery != q) {
        m_searchQuery = q; emit searchQueryChanged(); applySearchFilter();
    }
}
double LibraryController::totalDurationHours() const {
    double s = 0.0;
    for (const auto &v : m_tracks)
        s += v.toMap().value(QStringLiteral("duracion")).toDouble();
    return s / 3600.0;
}
void LibraryController::search(const QString &q) { setSearchQuery(q); }
void LibraryController::loadTracks(int page) {
    QString dir = Infra::RecursiveDirectoryScanner::resolveRootPath();
    if (!dir.isEmpty()) {
        m_isLoading = true; m_statusMessage = QStringLiteral("Buscando...");
        emit loadingChanged(); emit statusMessageChanged();
        m_allTracks.clear(); m_tracks.clear(); m_albums.clear();
        m_artists.clear(); m_knownAlbums.clear(); m_knownArtists.clear();
        emit tracksChanged(); m_scanner->startScan(dir); return;
    }

    if (!m_api) return;
    m_isLoading = true; emit loadingChanged();
    m_api->fetchTracks(page, 50, m_searchQuery, [this](bool ok, auto arr) {
        m_isLoading = false; emit loadingChanged();
        if (!ok) return;
        m_tracks.clear();
        for (const auto &val : arr)
            m_tracks.append(val.toObject().toVariantMap());
        emit tracksChanged();
    });
}
void LibraryController::loadAlbums(int page) {
    if (!m_albums.isEmpty() || !m_api) return;
    m_isLoading = true; emit loadingChanged();
    m_api->fetchAlbums(page, 50, [this](bool ok, auto arr) {
        m_isLoading = false; emit loadingChanged();
        if (!ok) return;
        m_albums.clear();
        for (const auto &val : arr)
            m_albums.append(val.toObject().toVariantMap());
        emit albumsChanged();
    });
}
void LibraryController::loadArtists(int page) {
    if (!m_artists.isEmpty() || !m_api) return;
    m_isLoading = true; emit loadingChanged();
    m_api->fetchArtists(page, 50, [this](bool ok, auto arr) {
        m_isLoading = false; emit loadingChanged();
        if (!ok) return;
        m_artists.clear();
        for (const auto &val : arr)
            m_artists.append(val.toObject().toVariantMap());
        emit artistsChanged();
    });
}
} // namespace Dopamine::Features::Library::App
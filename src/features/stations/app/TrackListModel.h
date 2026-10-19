/**
 * Location: src/features/stations/app/TrackListModel.h
 * Function: High performance QAbstractListModel managing audio tracks, metadata
 * formatting, search filtering, and multi-selection for batch operations.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_STATIONS_APP_TRACKLISTMODEL_H
#define DOPAMINE_FEATURES_STATIONS_APP_TRACKLISTMODEL_H

#include <QAbstractListModel>
#include <QSet>
#include "../domain/Track.h"

namespace Dopamine::Features::Stations::App {

class TrackListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        PathRole,
        TitleRole,
        ArtistRole,
        AlbumRole,
        DurationRole,
        DurationFormattedRole,
        FormatRole,
        GenreRole,
        StationIdRole,
        OriginRole,
        BpmRole,
        BitrateRole,
        SelectedRole
    };

    explicit TrackListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setTracks(const QList<Domain::Track> &tracks);
    void toggleSelection(int trackId);
    void selectAll(bool selected);
    QList<int> getSelectedTrackIds() const;
    int selectedCount() const { return m_selectedIds.size(); }

    Domain::Track getTrack(int row) const;
    Domain::Track getTrackById(int trackId) const;

signals:
    void selectionChanged();

private:
    QList<Domain::Track> m_tracks;
    QSet<int> m_selectedIds;
    QString formatDuration(double seconds) const;
};

} // namespace Dopamine::Features::Stations::App

#endif // DOPAMINE_FEATURES_STATIONS_APP_TRACKLISTMODEL_H

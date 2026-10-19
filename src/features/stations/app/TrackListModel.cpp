/**
 * Location: src/features/stations/app/TrackListModel.cpp
 * Function: Implementation of the TrackListModel providing virtualized track
 * rows, formatted times, selection states, and fast lookup by identifier.
 * All Rights Reserved isgaar
 */
#include "TrackListModel.h"
#include "shared/Theme.h"

namespace Dopamine::Features::Stations::App {

TrackListModel::TrackListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int TrackListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_tracks.size());
}

QString TrackListModel::formatDuration(double seconds) const {
    return Shared::Theme::instance().formatDuration(seconds);
}

QVariant TrackListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= m_tracks.size()) return QVariant();
    const auto &t = m_tracks.at(index.row());
    switch (role) {
    case IdRole: return t.id;
    case PathRole: return t.path;
    case TitleRole: return t.title.isEmpty() ? "Desconocido" : t.title;
    case ArtistRole: return t.artist.isEmpty() ? "Desconocido" : t.artist;
    case AlbumRole: return t.album;
    case DurationRole: return t.duration;
    case DurationFormattedRole: return formatDuration(t.duration);
    case FormatRole: return t.format.toUpper();
    case GenreRole: return t.genre;
    case StationIdRole: return t.stationId;
    case OriginRole: return t.origin;
    case BpmRole: return t.bpm;
    case BitrateRole: return t.bitrate;
    case SelectedRole: return m_selectedIds.contains(t.id);
    default: return QVariant();
    }
}

QHash<int, QByteArray> TrackListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "trackId";
    roles[PathRole] = "filePath";
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    roles[AlbumRole] = "album";
    roles[DurationRole] = "durationSec";
    roles[DurationFormattedRole] = "durationStr";
    roles[FormatRole] = "audioFormat";
    roles[GenreRole] = "genre";
    roles[StationIdRole] = "stationId";
    roles[OriginRole] = "origin";
    roles[BpmRole] = "bpm";
    roles[BitrateRole] = "bitrate";
    roles[SelectedRole] = "isSelected";
    return roles;
}

void TrackListModel::setTracks(const QList<Domain::Track> &tracks) {
    beginResetModel();
    m_tracks = tracks;
    m_selectedIds.clear();
    endResetModel();
    emit selectionChanged();
}

void TrackListModel::toggleSelection(int trackId) {
    if (m_selectedIds.contains(trackId)) {
        m_selectedIds.remove(trackId);
    } else {
        m_selectedIds.insert(trackId);
    }
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks.at(i).id == trackId) {
            emit dataChanged(index(i, 0), index(i, 0), {SelectedRole});
            break;
        }
    }
    emit selectionChanged();
}

void TrackListModel::selectAll(bool selected) {
    m_selectedIds.clear();
    if (selected) {
        for (const auto &t : m_tracks) m_selectedIds.insert(t.id);
    }
    if (!m_tracks.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_tracks.size() - 1, 0),
                         {SelectedRole});
    }
    emit selectionChanged();
}

QList<int> TrackListModel::getSelectedTrackIds() const {
    return m_selectedIds.values();
}

Domain::Track TrackListModel::getTrack(int row) const {
    if (row >= 0 && row < m_tracks.size()) return m_tracks.at(row);
    return Domain::Track();
}

Domain::Track TrackListModel::getTrackById(int trackId) const {
    for (const auto &t : m_tracks) {
        if (t.id == trackId) return t;
    }
    return Domain::Track();
}

} // namespace Dopamine::Features::Stations::App

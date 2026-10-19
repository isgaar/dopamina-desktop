/**
 * Location: src/features/stations/app/StationListModel.cpp
 * Function: Implementation of the StationListModel providing station items,
 * frequencies, active indicators, and track counters to UI delegates.
 * All Rights Reserved isgaar
 */
#include "StationListModel.h"

namespace Dopamine::Features::Stations::App {

StationListModel::StationListModel(QObject *parent)
    : QAbstractListModel(parent), m_activeStationId("electronica") {}

int StationListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_stations.size());
}

QVariant StationListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= m_stations.size()) {
        return QVariant();
    }
    const auto &s = m_stations.at(index.row());
    switch (role) {
    case IdRole: return s.id;
    case NameRole: return s.name;
    case FrequencyRole: return s.frequency;
    case GenreRole: return s.genre;
    case DescriptionRole: return s.description;
    case TrackCountRole: return s.trackCount;
    case DurationRole: return s.totalDurationSec;
    case DjModeRole: return s.djMode;
    case IsActiveRole:
        return s.id.compare(m_activeStationId, Qt::CaseInsensitive) == 0;
    default: return QVariant();
    }
}

QHash<int, QByteArray> StationListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "stationId";
    roles[NameRole] = "name";
    roles[FrequencyRole] = "frequency";
    roles[GenreRole] = "genre";
    roles[DescriptionRole] = "description";
    roles[TrackCountRole] = "trackCount";
    roles[DurationRole] = "totalDuration";
    roles[DjModeRole] = "djMode";
    roles[IsActiveRole] = "isActive";
    return roles;
}

void StationListModel::setStations(const QList<Domain::Station> &stations,
                                   const QString &activeStationId) {
    beginResetModel();
    m_stations = stations;
    m_activeStationId = activeStationId;
    endResetModel();
}

void StationListModel::setActiveStationId(const QString &activeStationId) {
    if (m_activeStationId == activeStationId) return;
    m_activeStationId = activeStationId;
    emit dataChanged(index(0, 0), index(m_stations.size() - 1, 0),
                     {IsActiveRole});
}

QVariantMap StationListModel::getStation(int row) const {
    if (row < 0 || row >= m_stations.size()) return {};
    const auto &s = m_stations.at(row);
    return {{"id", s.id}, {"name", s.name}, {"frequency", s.frequency},
            {"genre", s.genre}, {"trackCount", s.trackCount}};
}

QString StationListModel::getStationId(int row) const {
    if (row >= 0 && row < m_stations.size()) return m_stations.at(row).id;
    return {};
}

Domain::Station
StationListModel::getStationById(const QString &id) const {
    for (const auto &s : m_stations) {
        if (s.id.compare(id, Qt::CaseInsensitive) == 0) return s;
    }
    return Domain::Station();
}

} // namespace Dopamine::Features::Stations::App

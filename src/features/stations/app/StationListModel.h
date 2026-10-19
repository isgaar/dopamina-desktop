/**
 * Location: src/features/stations/app/StationListModel.h
 * Function: QAbstractListModel exposing the FM radio stations catalog, dials,
 * and aggregated track counts to declarative QML list views and sidebars.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_STATIONS_APP_STATIONLISTMODEL_H
#define DOPAMINE_FEATURES_STATIONS_APP_STATIONLISTMODEL_H

#include <QAbstractListModel>
#include "../domain/Station.h"

namespace Dopamine::Features::Stations::App {

class StationListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        FrequencyRole,
        GenreRole,
        DescriptionRole,
        TrackCountRole,
        DurationRole,
        DjModeRole,
        IsActiveRole
    };

    explicit StationListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setStations(const QList<Domain::Station> &stations,
                     const QString &activeStationId);
    void setActiveStationId(const QString &activeStationId);

    Q_INVOKABLE QVariantMap getStation(int row) const;
    Q_INVOKABLE QString getStationId(int row) const;
    Domain::Station getStationById(const QString &id) const;

private:
    QList<Domain::Station> m_stations;
    QString m_activeStationId;
};

} // namespace Dopamine::Features::Stations::App

#endif // DOPAMINE_FEATURES_STATIONS_APP_STATIONLISTMODEL_H

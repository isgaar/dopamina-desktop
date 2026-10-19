/**
 * Location: src/features/stations/domain/IStationRepository.h
 * Function: Pure domain interface abstraction for querying stations, counting
 * tracks, searching library items, and reassigning tracks to target stations.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_STATIONS_DOMAIN_ISTATIONREPOSITORY_H
#define DOPAMINE_FEATURES_STATIONS_DOMAIN_ISTATIONREPOSITORY_H

#include <QList>
#include <QString>
#include "Station.h"
#include "Track.h"

namespace Dopamine::Features::Stations::Domain {

class IStationRepository {
public:
    virtual ~IStationRepository() = default;

    virtual QList<Station> getStations() = 0;
    virtual QList<Track> getTracks(const QString &stationId,
                                  const QString &searchQuery,
                                  const QString &formatFilter) = 0;
    virtual bool reassignTrack(int trackId,
                              const QString &targetStationId) = 0;
    virtual bool reassignTracks(const QList<int> &trackIds,
                               const QString &targetStationId) = 0;
    virtual Station getStationStats(const QString &stationId) = 0;
    virtual int getTotalTrackCount() = 0;
};

} // namespace Dopamine::Features::Stations::Domain

#endif // DOPAMINE_FEATURES_STATIONS_DOMAIN_ISTATIONREPOSITORY_H

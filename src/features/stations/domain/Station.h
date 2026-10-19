/**
 * Location: src/features/stations/domain/Station.h
 * Function: Radio station entity defining transmission frequency, genre
 * profile, crossfade timings, DJ transition mode, and track count statistics.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_STATIONS_DOMAIN_STATION_H
#define DOPAMINE_FEATURES_STATIONS_DOMAIN_STATION_H

#include <QString>

namespace Dopamine::Features::Stations::Domain {

struct Station {
    QString id;
    QString name;
    QString frequency;
    QString genre;
    QString description;
    bool djMode = false;
    double crossfadeSec = 2.0;
    int trackCount = 0;
    double totalDurationSec = 0.0;
};

} // namespace Dopamine::Features::Stations::Domain

#endif // DOPAMINE_FEATURES_STATIONS_DOMAIN_STATION_H

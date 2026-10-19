/**
 * Location: src/features/stations/domain/Track.h
 * Function: Music track entity encapsulating audio metadata, tags, format,
 * assigned station identifier, BPM, bitrate, duration, and local file path.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_STATIONS_DOMAIN_TRACK_H
#define DOPAMINE_FEATURES_STATIONS_DOMAIN_TRACK_H

#include <QString>

namespace Dopamine::Features::Stations::Domain {

struct Track {
    int id = 0;
    QString path;
    QString title;
    QString artist;
    QString album;
    double duration = 0.0;
    QString format;
    QString genre;
    QString stationId;
    QString origin;
    double bpm = 0.0;
    int bitrate = 0;
    bool active = true;
    bool selected = false;
};

} // namespace Dopamine::Features::Stations::Domain

#endif // DOPAMINE_FEATURES_STATIONS_DOMAIN_TRACK_H

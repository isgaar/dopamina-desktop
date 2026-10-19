/**
 * Location: src/features/playback/domain/PlaybackTypes.h
 * Function: Domain models and state descriptors for playback track metadata
 * and active radio station configurations.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYBACK_DOMAIN_PLAYBACKTYPES_H
#define DOPAMINE_FEATURES_PLAYBACK_DOMAIN_PLAYBACKTYPES_H

#include <QString>

namespace Dopamine::Features::Playback::Domain {

struct ActiveStationInfo {
    QString id;
    QString name;
    QString dial;
    QString genre;
    QString desc;
    int trackCount = 0;
    bool isLive = false;
    bool hasTracks = true;
};

struct ActiveTrackInfo {
    int id = 0;
    QString title;
    QString artist;
    QString album;
    QString cover;
    int duration = 0;
};

} // namespace Dopamine::Features::Playback::Domain

#endif // DOPAMINE_FEATURES_PLAYBACK_DOMAIN_PLAYBACKTYPES_H

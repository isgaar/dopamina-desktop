/**
 * Location: src/features/playback/domain/PlaybackTypes.h
 * Function: Domain model describing the active library track metadata used
 * by the playback controller and transport views.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYBACK_DOMAIN_PLAYBACKTYPES_H
#define DOPAMINE_FEATURES_PLAYBACK_DOMAIN_PLAYBACKTYPES_H

#include <QString>

namespace Dopamine::Features::Playback::Domain {

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

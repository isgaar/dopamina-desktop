/**
 * Location: src/features/queue/domain/QueueTrack.h
 * Function: Data structure defining playback queue items with unique instance
 * tokens, track identifiers, song titles, artists, and radio flags.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_QUEUE_DOMAIN_QUEUETRACK_H
#define DOPAMINE_FEATURES_QUEUE_DOMAIN_QUEUETRACK_H

#include <QString>
#include <QVariantMap>

namespace Dopamine::Features::Queue::Domain {

struct QueueTrack {
    int id = 0;
    QString title;
    QString artist;
    QString album;
    int duration = 0;
    bool isFromRadio = false;

    QVariantMap toVariantMap() const {
        return {
            {"id", id},
            {"title", title},
            {"artist", artist},
            {"album", album},
            {"duration", duration},
            {"isRadio", isFromRadio}
        };
    }
};

} // namespace Dopamine::Features::Queue::Domain

#endif // DOPAMINE_FEATURES_QUEUE_DOMAIN_QUEUETRACK_H

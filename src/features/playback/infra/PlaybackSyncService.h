/**
 * Location: src/features/playback/infra/PlaybackSyncService.h
 * Function: Infrastructure orchestrator synchronizing station details, catalog
 * listings, and synchronized lyrics payloads across API requests.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYBACK_INFRA_PLAYBACKSYNCSERVICE_H
#define DOPAMINE_FEATURES_PLAYBACK_INFRA_PLAYBACKSYNCSERVICE_H

#include <QObject>
#include <QVariantList>
#include <functional>
#include "PlaybackApiClient.h"

namespace Dopamine::Features::Playback::Infra {

class PlaybackSyncService : public QObject {
    Q_OBJECT
public:
    explicit PlaybackSyncService(PlaybackApiClient *api,
                                QObject *parent = nullptr);
    ~PlaybackSyncService() override = default;

    using TuneCallback = std::function<
        void(bool ok, const Domain::ActiveStationInfo &st,
             const Domain::ActiveTrackInfo &tr,
             const QList<Domain::LrcLine> &l,
             const QVariantList &v,
             const QVariantList &tracks,
             const QVariantList &upcoming)>;

    using PollCallback = std::function<
        void(const Domain::ActiveTrackInfo &tr,
             const QList<Domain::LrcLine> &l,
             const QVariantList &v)>;

    void tune(const QString &stationId, TuneCallback cb);
    void poll(const QString &stationId, int curTrackId, PollCallback cb);

private:
    PlaybackApiClient *m_api = nullptr;
};

} // namespace Dopamine::Features::Playback::Infra

#endif // DOPAMINE_FEATURES_PLAYBACK_INFRA_PLAYBACKSYNCSERVICE_H

/**
 * Location: src/features/playback/infra/PlaybackApiClient.h
 * Function: Network client fetching remote station status, now-playing tracks,
 * catalog listings, and synchronized LRC lyrics from REST endpoints.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYBACK_INFRA_PLAYBACKAPICLIENT_H
#define DOPAMINE_FEATURES_PLAYBACK_INFRA_PLAYBACKAPICLIENT_H

#include <QObject>
#include <QVariantList>
#include <functional>
#include "../domain/PlaybackTypes.h"
#include "../domain/LrcParser.h"
#include "../../../core/ApiClient.h"

namespace Dopamine::Features::Playback::Infra {

class PlaybackApiClient : public QObject {
    Q_OBJECT
public:
    explicit PlaybackApiClient(Core::ApiClient *client,
                              QObject *parent = nullptr);
    ~PlaybackApiClient() override = default;

    using StationCallback = std::function<
        void(bool ok, const Domain::ActiveStationInfo &st,
             const Domain::ActiveTrackInfo &tr)>;
    using LyricsCallback = std::function<
        void(bool ok, const QList<Domain::LrcLine> &lines,
             const QVariantList &variant)>;
    using CatalogCallback =
        std::function<void(bool ok, const QVariantList &tracks,
                           const QVariantList &upcoming)>;
    using ActionCallback = std::function<void(bool ok)>;

    void fetchStation(const QString &stationId, StationCallback cb);
    void fetchLyrics(int trackId, LyricsCallback cb);
    void fetchTracks(const QString &genre, int curIdx, CatalogCallback cb);
    void requestNext(const QString &stationId, ActionCallback cb);

private:
    Core::ApiClient *m_client = nullptr;
};

} // namespace Dopamine::Features::Playback::Infra

#endif // DOPAMINE_FEATURES_PLAYBACK_INFRA_PLAYBACKAPICLIENT_H

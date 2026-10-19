/**
 * Location: src/features/stations/infra/StationHttpService.h
 * Function: HTTP service adapter communicating with the DopamineFM FastAPI
 * backend to trigger VLC streaming, advance tracks, and query live metadata.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_STATIONS_INFRA_STATIONHTTPSERVICE_H
#define DOPAMINE_FEATURES_STATIONS_INFRA_STATIONHTTPSERVICE_H

#include <QObject>
#include <functional>
#include "core/ApiClient.h"

namespace Dopamine::Features::Stations::Infra {

class StationHttpService : public QObject {
    Q_OBJECT
public:
    explicit StationHttpService(Core::ApiClient *client,
                                QObject *parent = nullptr);

    void skipNextTrack(const QString &stationId,
                       std::function<void(bool ok)> callback);
    void fetchNowPlaying(
        const QString &stationId,
        std::function<void(int trackId, const QString &title)> callback);

private:
    Core::ApiClient *m_client;
};

} // namespace Dopamine::Features::Stations::Infra

#endif // DOPAMINE_FEATURES_STATIONS_INFRA_STATIONHTTPSERVICE_H

/**
 * Location: src/features/interconnect/infra/SessionPlaybackApiClient.h
 * Function: Network adapter reading remote playback state and resolving active
 * queue ids into streamable track DTOs for desktop session mirroring.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_INTERCONNECT_INFRA_SESSIONPLAYBACKAPICLIENT_H
#define DOPAMINE_FEATURES_INTERCONNECT_INFRA_SESSIONPLAYBACKAPICLIENT_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <functional>
#include "core/ApiClient.h"

namespace Dopamine::Features::Interconnect::Infra {

class SessionPlaybackApiClient : public QObject {
    Q_OBJECT
public:
    explicit SessionPlaybackApiClient(Core::ApiClient *client,
                                      QObject *parent = nullptr);

    using StateCallback =
        std::function<void(bool ok, const QJsonObject &state)>;
    using TracksCallback =
        std::function<void(bool ok, const QJsonArray &tracks)>;

    virtual void fetchState(const QString &userId, StateCallback cb);
    virtual void fetchSavedSession(const QString &userId, StateCallback cb);
    virtual void resolveTracks(const QList<int> &ids, TracksCallback cb);
    virtual void publishState(const QJsonObject &payload);
    virtual void saveSession(const QJsonObject &payload);

private:
    Core::ApiClient *m_client = nullptr;
};

} // namespace Dopamine::Features::Interconnect::Infra

#endif // DOPAMINE_FEATURES_INTERCONNECT_INFRA_SESSIONPLAYBACKAPICLIENT_H

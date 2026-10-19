/**
 * Location: src/features/queue/infra/QueueApiClient.h
 * Function: Network client requesting stateless song radio recommendation
 * batches and posting playback history events to server endpoints.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_QUEUE_INFRA_QUEUEAPICLIENT_H
#define DOPAMINE_FEATURES_QUEUE_INFRA_QUEUEAPICLIENT_H

#include <QObject>
#include <QJsonArray>
#include <functional>
#include "core/ApiClient.h"

namespace Dopamine::Features::Queue::Infra {

class QueueApiClient : public QObject {
    Q_OBJECT
public:
    explicit QueueApiClient(Core::ApiClient *client,
                            QObject *parent = nullptr);

    using RadioCallback =
        std::function<void(bool ok, const QJsonArray &trackIds)>;

    void fetchRadioNext(int seedTrackId,
                        const QList<int> &historyIds,
                        const QJsonArray &events,
                        const QString &mode,
                        int count,
                        RadioCallback cb);

    void recordHistory(int trackId);

private:
    Core::ApiClient *m_client = nullptr;
};

} // namespace Dopamine::Features::Queue::Infra

#endif // DOPAMINE_FEATURES_QUEUE_INFRA_QUEUEAPICLIENT_H

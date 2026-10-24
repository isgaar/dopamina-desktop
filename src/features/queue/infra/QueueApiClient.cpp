/**
 * Location: src/features/queue/infra/QueueApiClient.cpp
 * Function: Implementation of mix recommendation API client packing JSON
 * payloads for stateless mix sequencer generation and playback auditing.
 * All Rights Reserved isgaar
 */
#include "QueueApiClient.h"
#include <QJsonObject>
#include <QJsonDocument>

namespace Dopamine::Features::Queue::Infra {

QueueApiClient::QueueApiClient(Core::ApiClient *client, QObject *parent)
    : QObject(parent), m_client(client) {}

void QueueApiClient::fetchMixNext(int seedTrackId,
                                    const QList<int> &historyIds,
                                    const QJsonArray &events,
                                    const QString &mode,
                                    int count,
                                    MixCallback cb) {
    if (!m_client) { if (cb) cb(false, {}); return; }
    QJsonObject payload;
    payload["seed_track_id"] = seedTrackId;
    QJsonArray histArr;
    for (int id : historyIds) histArr.append(id);
    payload["historial_ids"] = histArr;
    payload["eventos"] = events;
    payload["modo"] = mode.isEmpty() ? "normal" : mode;
    payload["n"] = count;

    QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    m_client->post("/api/v1/mix/next", body, [cb](bool ok, auto data) {
        if (!ok || !cb) { if (cb) cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            auto arr = doc.object().value("ids").toArray();
            if (arr.isEmpty())
                arr = doc.object().value("track_ids").toArray();
            cb(true, arr);
        } else {
            cb(false, {});
        }
    });
}

void QueueApiClient::recordHistory(int trackId) {
    if (!m_client || trackId <= 0) return;
    QJsonObject payload; payload["cancion_id"] = trackId;
    QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    m_client->post("/api/v1/history", body, [](bool, auto) {});
}

} // namespace Dopamine::Features::Queue::Infra

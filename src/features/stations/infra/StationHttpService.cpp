/**
 * Location: src/features/stations/infra/StationHttpService.cpp
 * Function: Implementation of the HTTP service executing requests to the
 * DopamineFM REST API for VLC playback control and next track progression.
 * All Rights Reserved isgaar
 */
#include "StationHttpService.h"
#include "core/ProjectConfig.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace Dopamine::Features::Stations::Infra {

StationHttpService::StationHttpService(Core::ApiClient *client,
                                       QObject *parent)
    : QObject(parent), m_client(client) {}

void StationHttpService::skipNextTrack(const QString &stationId,
                                       std::function<void(bool ok)> callback) {
    if (!m_client) {
        if (callback) callback(false);
        return;
    }
    QString target = (stationId == "all" || stationId == "unassigned" ||
                      stationId.isEmpty()) ? QStringLiteral("variado")
                                           : stationId;
    QString url = QString("%1/stations/%2/next")
        .arg(Core::ProjectConfig::instance().apiUrl(), target);
    m_client->post(url, QByteArray(), [callback](bool ok, const QByteArray &) {
        if (callback) callback(ok);
    });
}

void StationHttpService::fetchNowPlaying(
    const QString &stationId,
    std::function<void(int trackId, const QString &title)> callback) {
    if (!m_client) return;
    QString target = (stationId == "all" || stationId == "unassigned" ||
                      stationId.isEmpty()) ? QStringLiteral("variado")
                                           : stationId;
    QString url = QString("%1/stations/%2/now-playing")
        .arg(Core::ProjectConfig::instance().apiUrl(), target);
    m_client->get(url, [callback](bool ok, const QByteArray &data) {
        if (!ok || !callback) return;
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject np = doc.object().value("now_playing").toObject();
            callback(np.value("id").toInt(),
                     np.value("display_name").toString());
        }
    });
}

} // namespace Dopamine::Features::Stations::Infra

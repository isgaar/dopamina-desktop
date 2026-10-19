/**
 * Location: src/features/playback/infra/PlaybackApiClient.cpp
 * Function: Implementation of the remote playback client handling API calls
 * for radio stations, synchronized lyrics, and track catalog queries.
 * All Rights Reserved isgaar
 */
#include "PlaybackApiClient.h"
#include "../../../core/ProjectConfig.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Dopamine::Features::Playback::Infra {

PlaybackApiClient::PlaybackApiClient(Core::ApiClient *client, QObject *parent)
    : QObject(parent), m_client(client) {}

void PlaybackApiClient::fetchStation(const QString &stationId,
                                     StationCallback cb) {
    if (!m_client) { cb(false, {}, {}); return; }
    QString url = Core::ProjectConfig::instance().apiStationsUrl()
        + "/" + stationId;
    m_client->get(url, [cb](bool ok, const QByteArray &data) {
        if (!ok) { cb(false, {}, {}); return; }
        QJsonObject o = QJsonDocument::fromJson(data).object();
        Domain::ActiveStationInfo st;
        st.id = o["id"].toString();
        st.name = o.value("nombre").toString(o["name"].toString());
        st.dial = o.value("frecuencia").toString(o["frequency"].toString());
        st.genre = o.value("genero").toString(o["genre"].toString());
        st.desc = o.value("descripcion").toString(o["description"].toString());
        QJsonObject np = o["now_playing"].toObject();
        st.hasTracks = !np.isEmpty();
        st.isLive = o.value("is_live").toBool(st.hasTracks);
        Domain::ActiveTrackInfo tr;
        tr.id = np["id"].toInt();
        tr.title = np.value("titulo").toString(np["title"].toString());
        tr.artist = np.value("artista").toString(np["artist"].toString());
        tr.album = np["album"].toString();
        tr.cover = o.value("cover_url").toString();
        tr.duration = static_cast<int>(np["duracion"].toDouble());
        cb(true, st, tr);
    });
}

void PlaybackApiClient::fetchLyrics(int trackId, LyricsCallback cb) {
    if (!m_client || trackId <= 0) { cb(false, {}, {}); return; }
    QString url = Core::ProjectConfig::instance().apiUrl()
        + QString("/api/v1/catalog/tracks/%1/lyrics").arg(trackId);
    m_client->get(url, [cb](bool ok, const QByteArray &data) {
        if (!ok) { cb(false, {}, {}); return; }
        QJsonObject o = QJsonDocument::fromJson(data).object();
        QList<Domain::LrcLine> lines;
        for (const auto &val : o["lines"].toArray()) {
            QJsonObject l = val.toObject();
            lines.append({l["time_ms"].toVariant().toLongLong(),
                          l["text"].toString()});
        }
        if (lines.isEmpty() && o.contains("raw")) {
            lines = Domain::LrcParser::parse(o["raw"].toString());
        }
        QVariantList v;
        for (const auto &l : lines) {
            v.append(QVariantMap{{"timeMs", l.timeMs}, {"text", l.text}});
        }
        cb(true, lines, v);
    });
}

void PlaybackApiClient::fetchTracks(const QString &genre, int curIdx,
                                    CatalogCallback cb) {
    if (!m_client) { cb(false, {}, {}); return; }
    QString url = Core::ProjectConfig::instance().apiUrl()
        + "/api/v1/catalog/tracks?limit=30";
    if (!genre.isEmpty()) url += "&genre=" + genre;
    m_client->get(url, [curIdx, cb](bool ok, const QByteArray &data) {
        if (!ok) { cb(false, {}, {}); return; }
        QJsonArray arr = QJsonDocument::fromJson(data).array();
        QVariantList tracks, upcoming;
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject o = arr[i].toObject();
            QVariantMap m;
            m["id"] = o["id"].toInt();
            m["title"] = o["titulo"].toString();
            m["artist"] = o["artista"].toString();
            m["album"] = o["album"].toString();
            m["duration"] = static_cast<int>(o["duracion"].toDouble());
            m["format"] = o["formato"].toString().toUpper();
            tracks.append(m);
        }
        int total = tracks.size();
        if (total > 0) {
            for (int k = 1; k <= 4; ++k) {
                upcoming.append(tracks[(curIdx + k) % total]);
            }
        }
        cb(true, tracks, upcoming);
    });
}

void PlaybackApiClient::requestNext(const QString &stationId,
                                    ActionCallback cb) {
    if (!m_client) { cb(false); return; }
    QString url = Core::ProjectConfig::instance().apiStationsUrl()
        + "/" + stationId + "/next";
    m_client->post(url, "{}", [cb](bool ok, const QByteArray &) { cb(ok); });
}

} // namespace Dopamine::Features::Playback::Infra

/**
 * Location: src/features/stations/app/StationController.cpp
 * Function: Implementation of the StationController coordinating station
 * navigation, track browsing, instant metadata lookup, and reassignments.
 * All Rights Reserved isgaar
 */
#include "StationController.h"
#include "shared/Theme.h"
#include "core/ProjectConfig.h"
#include "core/audio/IAudioBackend.h"
#include <QUrl>
namespace Dopamine::Features::Stations::App {
StationController::StationController(Domain::IStationRepository *repo,
                                     Infra::StationHttpService *httpService,
                                     QObject *parent)
    : QObject(parent), m_repo(repo), m_http(httpService),
      m_stationsModel(this), m_tracksModel(this) { reload(); }
void StationController::reload() { loadStations(); loadTracks(); }
static Domain::Station resolveStation(const QString &id,
                                      const StationListModel &m, int n) {
    if (id == "all") return {"all", "Toda la Biblioteca", "", "variado",
        "Colección completa de pistas", false, 0.0, n, 0.0};
    if (id == "unassigned") return {"unassigned", "Sin Asignar", "",
        "unassigned", "Pistas pendientes de asignación", false, 0.0, 0, 0.0};
    return m.getStationById(id);
}
void StationController::loadStations() {
    if (!m_repo) return;
    m_stationsModel.setStations(m_repo->getStations(), m_currentStationId);
    m_totalLibraryTracks = m_repo->getTotalTrackCount();
    m_currentStation = resolveStation(m_currentStationId, m_stationsModel,
                                      m_totalLibraryTracks);
    emit stationChanged(); emit statsChanged();
}
void StationController::loadTracks() {
    if (!m_repo) return;
    auto t = m_repo->getTracks(
        m_currentStationId, m_searchQuery, m_formatFilter);
    m_tracksModel.setTracks(t);
    if (!t.isEmpty()) {
        inspectTrack(t.first().id);
        if (m_isPlaying) syncNowPlaying();
    } else {
        m_inspectedTrack = Domain::Track(); emit trackChanged();
    }
}
QString StationController::currentStationDuration() const {
    int t = static_cast<int>(m_currentStation.totalDurationSec);
    int h = t / 3600, m = (t % 3600) / 60;
    return (h > 0) ? QString("%1 h %2 min").arg(h).arg(m)
                   : QString("%1 min").arg(m);
}
QString StationController::inspectedDuration() const {
    return Shared::Theme::instance().formatDuration(m_inspectedTrack.duration);
}
void StationController::selectStation(const QString &stationId) {
    if (m_currentStationId == stationId) return;
    m_currentStationId = stationId;
    m_stationsModel.setActiveStationId(stationId);
    m_currentStation = resolveStation(stationId, m_stationsModel,
                                      m_totalLibraryTracks);
    emit stationChanged(); loadTracks();
    if (m_isPlaying) playCurrentStation();
}
void StationController::search(const QString &query) {
    if (m_searchQuery == query) return;
    m_searchQuery = query; emit searchChanged(); loadTracks();
}
void StationController::setFormatFilter(const QString &format) {
    if (m_formatFilter == format) return;
    m_formatFilter = format; emit filterChanged(); loadTracks();
}
void StationController::inspectTrack(int trackId) {
    m_inspectedTrack = m_tracksModel.getTrackById(trackId);
    emit trackChanged();
}
void StationController::toggleTrackSelection(int trackId) {
    m_tracksModel.toggleSelection(trackId);
}
void StationController::selectAllTracks(bool select) {
    m_tracksModel.selectAll(select);
}
bool StationController::reassignSelectedTracks(const QString &targetId) {
    if (!m_repo) return false;
    auto ids = m_tracksModel.getSelectedTrackIds();
    return (!ids.isEmpty() && m_repo->reassignTracks(ids, targetId))
        ? (reload(), true) : false;
}
bool StationController::reassignTrack(int trackId, const QString &targetId) {
    return (m_repo && m_repo->reassignTrack(trackId, targetId))
        ? (reload(), true) : false;
}
void StationController::playCurrentStation() {
    if (m_audio) {
        QString u = Core::ProjectConfig::instance()
            .streamUrl(m_currentStationId);
        m_audio->play(QUrl(u));
        m_isPlaying = true;
        emit playStateChanged();
        syncNowPlaying();
    }
}
void StationController::stopStation() {
    if (m_audio) {
        m_audio->stop(); m_isPlaying = false; emit playStateChanged();
    }
}
void StationController::syncNowPlaying() {
    if (!m_http) return;
    m_http->fetchNowPlaying(m_currentStationId,
        [this](int id, const QString &) {
            if (id > 0 && m_tracksModel.getTrackById(id).id == id)
                inspectTrack(id);
        });
}

} // namespace Dopamine::Features::Stations::App

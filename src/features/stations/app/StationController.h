/**
 * Location: src/features/stations/app/StationController.h
 * Function: Master application controller orchestrating station browsing,
 * dynamic search queries, track selections, reassignments, and stream playback.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_STATIONS_APP_STATIONCONTROLLER_H
#define DOPAMINE_FEATURES_STATIONS_APP_STATIONCONTROLLER_H

#include <QObject>
#include <QString>
#include "StationListModel.h"
#include "TrackListModel.h"
#include "../domain/IStationRepository.h"
#include "../infra/StationHttpService.h"

namespace Dopamine::Core::Audio { class IAudioBackend; }
namespace Dopamine::Features::Stations::App {

class StationController : public QObject {
    Q_OBJECT
    Q_PROPERTY(StationListModel* stationsModel READ stationsModel CONSTANT)
    Q_PROPERTY(TrackListModel* tracksModel READ tracksModel CONSTANT)
    Q_PROPERTY(QString currentStationId READ currentStationId
               NOTIFY stationChanged)
    Q_PROPERTY(QString currentStationName READ currentStationName
               NOTIFY stationChanged)
    Q_PROPERTY(QString currentStationDial READ currentStationDial
               NOTIFY stationChanged)
    Q_PROPERTY(QString currentStationDesc READ currentStationDesc
               NOTIFY stationChanged)
    Q_PROPERTY(int currentStationTrackCount READ currentStationTrackCount
               NOTIFY stationChanged)
    Q_PROPERTY(QString currentStationDuration READ currentStationDuration
               NOTIFY stationChanged)
    Q_PROPERTY(bool currentStationDjMode READ currentStationDjMode
               NOTIFY stationChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery NOTIFY searchChanged)
    Q_PROPERTY(QString formatFilter READ formatFilter NOTIFY filterChanged)
    Q_PROPERTY(int totalLibraryTracks READ totalLibraryTracks
               NOTIFY statsChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playStateChanged)
    Q_PROPERTY(int inspectedTrackId READ inspectedTrackId NOTIFY trackChanged)
    Q_PROPERTY(QString inspectedTitle READ inspectedTitle NOTIFY trackChanged)
    Q_PROPERTY(QString inspectedArtist READ inspectedArtist
               NOTIFY trackChanged)
    Q_PROPERTY(QString inspectedAlbum READ inspectedAlbum NOTIFY trackChanged)
    Q_PROPERTY(QString inspectedPath READ inspectedPath NOTIFY trackChanged)
    Q_PROPERTY(QString inspectedFormat READ inspectedFormat NOTIFY trackChanged)
    Q_PROPERTY(QString inspectedDuration READ inspectedDuration
               NOTIFY trackChanged)
    Q_PROPERTY(double inspectedBpm READ inspectedBpm NOTIFY trackChanged)
    Q_PROPERTY(int inspectedBitrate READ inspectedBitrate NOTIFY trackChanged)

public:
    explicit StationController(Domain::IStationRepository *repo,
                              Infra::StationHttpService *httpService,
                              QObject *parent = nullptr);

    StationListModel* stationsModel() { return &m_stationsModel; }
    TrackListModel* tracksModel() { return &m_tracksModel; }
    QString currentStationId() const { return m_currentStationId; }
    QString currentStationName() const { return m_currentStation.name; }
    QString currentStationDial() const { return m_currentStation.frequency; }
    QString currentStationDesc() const { return m_currentStation.description; }
    int currentStationTrackCount() const { return m_currentStation.trackCount; }
    QString currentStationDuration() const;
    bool currentStationDjMode() const { return m_currentStation.djMode; }
    QString searchQuery() const { return m_searchQuery; }
    QString formatFilter() const { return m_formatFilter; }
    int totalLibraryTracks() const { return m_totalLibraryTracks; }
    bool isPlaying() const { return m_isPlaying; }
    int inspectedTrackId() const { return m_inspectedTrack.id; }
    QString inspectedTitle() const { return m_inspectedTrack.title; }
    QString inspectedArtist() const { return m_inspectedTrack.artist; }
    QString inspectedAlbum() const { return m_inspectedTrack.album; }
    QString inspectedPath() const { return m_inspectedTrack.path; }
    QString inspectedFormat() const {
        return m_inspectedTrack.format.toUpper();
    }
    QString inspectedDuration() const;
    double inspectedBpm() const { return m_inspectedTrack.bpm; }
    int inspectedBitrate() const { return m_inspectedTrack.bitrate; }
    void setAudioBackend(Core::Audio::IAudioBackend *b) { m_audio = b; }
    Q_INVOKABLE void selectStation(const QString &stationId);
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void setFormatFilter(const QString &format);
    Q_INVOKABLE void inspectTrack(int trackId);
    Q_INVOKABLE void toggleTrackSelection(int trackId);
    Q_INVOKABLE void selectAllTracks(bool select);
    Q_INVOKABLE bool reassignSelectedTracks(const QString &targetStationId);
    Q_INVOKABLE bool reassignTrack(int trackId, const QString &targetStationId);
    Q_INVOKABLE void playCurrentStation();
    Q_INVOKABLE void stopStation();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void syncNowPlaying();

signals:
    void stationChanged(); void searchChanged(); void filterChanged();
    void statsChanged(); void playStateChanged(); void trackChanged();

private:
    void loadStations(); void loadTracks();
    Domain::IStationRepository *m_repo = nullptr;
    Infra::StationHttpService *m_http = nullptr;
    Core::Audio::IAudioBackend *m_audio = nullptr;
    StationListModel m_stationsModel;
    TrackListModel m_tracksModel;
    QString m_currentStationId = "electronica";
    Domain::Station m_currentStation;
    Domain::Track m_inspectedTrack;
    QString m_searchQuery, m_formatFilter = "all";
    int m_totalLibraryTracks = 0;
    bool m_isPlaying = false;
};

} // namespace Dopamine::Features::Stations::App

#endif // DOPAMINE_FEATURES_STATIONS_APP_STATIONCONTROLLER_H

/**
 * Location: src/features/playback/infra/PlaybackSyncService.cpp
 * Function: Implementation of playback synchronization orchestrating station
 * fetching, lyrics loading, and catalog track updates.
 * All Rights Reserved isgaar
 */
#include "PlaybackSyncService.h"

namespace Dopamine::Features::Playback::Infra {

PlaybackSyncService::PlaybackSyncService(PlaybackApiClient *api,
                                         QObject *parent)
    : QObject(parent), m_api(api) {}

void PlaybackSyncService::tune(const QString &stationId, TuneCallback cb) {
    if (!m_api) { cb(false, {}, {}, {}, {}, {}, {}); return; }
    m_api->fetchStation(stationId, [this, cb](bool ok, auto st, auto tr) {
        if (!ok) { cb(false, {}, {}, {}, {}, {}, {}); return; }
        m_api->fetchTracks(st.genre, 0,
            [this, cb, st, tr](bool, auto t, auto u) {
                if (tr.id > 0) {
                    m_api->fetchLyrics(tr.id,
                        [cb, st, tr, t, u](bool, auto l, auto v) {
                            cb(true, st, tr, l, v, t, u);
                        });
                } else {
                    cb(true, st, tr, {}, {}, t, u);
                }
            });
    });
}

void PlaybackSyncService::poll(const QString &stationId, int curTrackId,
                               PollCallback cb) {
    if (!m_api || stationId.isEmpty()) return;
    m_api->fetchStation(stationId,
        [this, curTrackId, cb](bool ok, auto, auto tr) {
            if (!ok || tr.id == curTrackId || tr.id <= 0) return;
            m_api->fetchLyrics(tr.id, [cb, tr](bool, auto l, auto v) {
                cb(tr, l, v);
            });
        });
}

} // namespace Dopamine::Features::Playback::Infra

/**
 * Location: src/core/daemon/AmbiguityDaemon.h
 * Function: Background thread worker resolving genre ambiguity on L0-L3 tiers.
 * All Rights Reserved isgaar
 */
#pragma once
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <vector>
#include "AmbiguitySupervisor.h"

namespace Dopamine::Core::Daemon {

enum class TaskPriority {
    P0_Seed = 0, P1_Queue = 1, P2_Playlist = 2, P3_Library = 3
};

struct AmbiguityTask {
    int trackId;
    TaskPriority priority;
};

class AmbiguityDaemon : public QThread {
    Q_OBJECT
public:
    explicit AmbiguityDaemon(QObject *parent = nullptr);
    ~AmbiguityDaemon() override;

    void enqueue(int trackId, TaskPriority priority);
    void setPlaybackRisk(bool atRisk);
    void setOffline(bool isOffline);
    bool isPaused() const;
    int pendingCount();

signals:
    void ambiguityResolved(int trackId, const QString &station, double conf);
    void questionReady(int trackId, const QString &questionJson);
    void daemonPaused(bool paused);

protected:
    void run() override;

private:
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    std::vector<AmbiguityTask> m_tasks;
    bool m_running{true};
    bool m_playbackRisk{false};
    bool m_offline{false};
    AmbiguitySupervisor m_supervisor;
};

} // namespace Dopamine::Core::Daemon

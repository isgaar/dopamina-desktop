/**
 * Location: src/core/daemon/AmbiguityDaemon.cpp
 * Function: Thread loop dispatching ambiguity tasks with pause gates.
 * All Rights Reserved isgaar
 */
#include "AmbiguityDaemon.h"
#include <algorithm>

namespace Dopamine::Core::Daemon {

AmbiguityDaemon::AmbiguityDaemon(QObject *parent)
    : QThread(parent) {}

AmbiguityDaemon::~AmbiguityDaemon() {
    {
        QMutexLocker lock(&m_mutex);
        m_running = false;
        m_condition.wakeAll();
    }
    wait();
}

void AmbiguityDaemon::enqueue(int trackId, TaskPriority priority) {
    QMutexLocker lock(&m_mutex);
    m_tasks.push_back({trackId, priority});
    std::sort(m_tasks.begin(), m_tasks.end(), [](const auto &a, const auto &b) {
        return static_cast<int>(a.priority) < static_cast<int>(b.priority);
    });
    m_condition.wakeOne();
}

void AmbiguityDaemon::setPlaybackRisk(bool atRisk) {
    QMutexLocker lock(&m_mutex);
    if (m_playbackRisk != atRisk) {
        m_playbackRisk = atRisk;
        emit daemonPaused(isPaused());
        if (!isPaused()) m_condition.wakeOne();
    }
}

void AmbiguityDaemon::setOffline(bool isOffline) {
    QMutexLocker lock(&m_mutex);
    if (m_offline != isOffline) {
        m_offline = isOffline;
        emit daemonPaused(isPaused());
        if (!isPaused()) m_condition.wakeOne();
    }
}

bool AmbiguityDaemon::isPaused() const {
    return m_playbackRisk || m_offline;
}

int AmbiguityDaemon::pendingCount() {
    QMutexLocker lock(&m_mutex);
    return static_cast<int>(m_tasks.size());
}

void AmbiguityDaemon::run() {
    while (true) {
        AmbiguityTask task;
        {
            QMutexLocker lock(&m_mutex);
            while (m_running && (m_tasks.empty() || isPaused())) {
                m_condition.wait(&m_mutex);
            }
            if (!m_running) break;
            task = m_tasks.front();
            m_tasks.erase(m_tasks.begin());
        }

        // Simula procesamiento determinista L0-L3
        emit ambiguityResolved(task.trackId, "urbano-espanol", 0.85);
    }
}

} // namespace Dopamine::Core::Daemon

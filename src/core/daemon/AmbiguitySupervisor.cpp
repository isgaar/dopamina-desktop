/**
 * Location: src/core/daemon/AmbiguitySupervisor.cpp
 * Function: Implementation of rate-limited supervisor restarting worker thread.
 * All Rights Reserved isgaar
 */
#include "AmbiguitySupervisor.h"
#include <algorithm>

namespace Dopamine::Core::Daemon {

AmbiguitySupervisor::AmbiguitySupervisor(QObject *parent)
    : QObject(parent) {}

bool AmbiguitySupervisor::recordCrashAndShouldRestart(qint64 nowSec) {
    qint64 cutoff = nowSec - kWindowSec;
    m_restarts.erase(
        std::remove_if(m_restarts.begin(), m_restarts.end(),
                       [cutoff](qint64 t) { return t < cutoff; }),
        m_restarts.end());

    if (static_cast<int>(m_restarts.size()) < kMaxRestarts) {
        m_restarts.push_back(nowSec);
        return true;
    }
    emit fatalFailureOccurred(
        "Límite de 3 reinicios en 10 min superado para AmbiguityDaemon.");
    return false;
}

void AmbiguitySupervisor::reset() {
    m_restarts.clear();
}

} // namespace Dopamine::Core::Daemon

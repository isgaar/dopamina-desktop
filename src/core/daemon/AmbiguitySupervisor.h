/**
 * Location: src/core/daemon/AmbiguitySupervisor.h
 * Function: Crash supervisor and restart controller for AmbiguityDaemon tasks.
 * All Rights Reserved isgaar
 */
#pragma once
#include <QObject>
#include <QDateTime>
#include <vector>

namespace Dopamine::Core::Daemon {

class AmbiguitySupervisor : public QObject {
    Q_OBJECT
public:
    explicit AmbiguitySupervisor(QObject *parent = nullptr);
    bool recordCrashAndShouldRestart(qint64 nowSec);
    int restartCount() const { return static_cast<int>(m_restarts.size()); }
    void reset();

signals:
    void fatalFailureOccurred(const QString &reason);

private:
    static constexpr int kMaxRestarts = 3;
    static constexpr qint64 kWindowSec = 600;
    std::vector<qint64> m_restarts;
};

} // namespace Dopamine::Core::Daemon

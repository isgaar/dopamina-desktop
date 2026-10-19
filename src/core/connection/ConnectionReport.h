/**
 * Location: src/core/connection/ConnectionReport.h
 * Function: Data transfer object describing connectivity level (A/B/C),
 * transport type, latency, failed probe index, and redacted error diagnostics.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_CONNECTION_CONNECTIONREPORT_H
#define DOPAMINE_CORE_CONNECTION_CONNECTIONREPORT_H

#include "core/errors/ErrorCode.h"
#include <QDateTime>
#include <QString>

namespace Core::Connection {

enum class ServiceLevel {
    Full = 0,    // Level A: Full (SMB + API)
    SmbOnly = 1, // Level B: SMB-only (API down)
    CacheOnly = 2 // Level C: Cache / Offline
};

struct ConnectionReport {
    ServiceLevel level{ServiceLevel::CacheOnly};
    QString transport{QStringLiteral("None")};
    qint64 latencyMs{0};
    int failedProbe{-1}; // 0..7 or -1 for success
    Core::Errors::ErrorCode code{Core::Errors::ErrorCode::NoError};
    QString redactedDetail;
    QDateTime timestamp{QDateTime::currentDateTimeUtc()};

    bool isHealthy() const {
        return level == ServiceLevel::Full
            && code == Core::Errors::ErrorCode::NoError;
    }
};

} // namespace Core::Connection

#endif // DOPAMINE_CORE_CONNECTION_CONNECTIONREPORT_H

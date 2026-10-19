/**
 * Location: src/core/connection/ProbeRunner.h
 * Function: Execution engine for diagnostic probes P0-P7 verifying network,
 * DNS, TCP ports, SMB authentication, storage shares, and REST API health.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_CONNECTION_PROBERUNNER_H
#define DOPAMINE_CORE_CONNECTION_PROBERUNNER_H

#include <QObject>
#include "ConnectionReport.h"
#include "core/storage/IStorageProvider.h"
#include "core/ApiClient.h"

namespace Core::Connection {

class ProbeRunner : public QObject {
    Q_OBJECT
public:
    explicit ProbeRunner(QObject* parent = nullptr);

    ConnectionReport runProbes(
        const QString& host,
        int port,
        const QString& share,
        const QString& folder,
        const QString& user,
        const QString& password,
        Dopamine::Core::Storage::IStorageProvider* storage,
        Dopamine::Core::ApiClient* api
    );

    static bool checkP0Network();
    static bool checkP1Dns(const QString& host, QString* outIp = nullptr);
    static bool checkP2Tcp(const QString& host, int port, int timeoutMs = 3000);
};

} // namespace Core::Connection

#endif // DOPAMINE_CORE_CONNECTION_PROBERUNNER_H

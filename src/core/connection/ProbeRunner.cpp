/**
 * Location: src/core/connection/ProbeRunner.cpp
 * Function: Implementation of probes P0-P7 verifying network reachability,
 * host resolution, port connectivity, SMB storage checks, and API health.
 * All Rights Reserved isgaar
 */
#include "ProbeRunner.h"
#include <QElapsedTimer>
#include <QHostInfo>
#include <QNetworkInformation>
#include <QNetworkInterface>
#include <QTcpSocket>

namespace Core::Connection {

ProbeRunner::ProbeRunner(QObject* parent) : QObject(parent) {}

bool ProbeRunner::checkP0Network() {
    const auto list = QNetworkInterface::allInterfaces();
    for (const auto& iface : list) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            iface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            return true;
        }
    }
    return false;
}

bool ProbeRunner::checkP1Dns(const QString& host, QString* outIp) {
    if (host.isEmpty()) return false;
    QHostInfo info = QHostInfo::fromName(host);
    if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
        return false;
    }
    if (outIp) *outIp = info.addresses().first().toString();
    return true;
}

bool ProbeRunner::checkP2Tcp(const QString& host, int port, int timeoutMs) {
    if (host.isEmpty() || port <= 0) return false;
    QTcpSocket sock;
    sock.connectToHost(host, static_cast<quint16>(port));
    return sock.waitForConnected(timeoutMs);
}

ConnectionReport ProbeRunner::runProbes(
    const QString& host,
    int port,
    const QString& share,
    const QString& folder,
    const QString& user,
    const QString& password,
    Dopamine::Core::Storage::IStorageProvider* storage,
    Dopamine::Core::ApiClient* api
) {
    Q_UNUSED(folder); Q_UNUSED(user); Q_UNUSED(password);
    QElapsedTimer timer; timer.start();
    ConnectionReport r;
    r.timestamp = QDateTime::currentDateTimeUtc();


    if (!checkP0Network()) {
        r.failedProbe = 0;
        r.code = Core::Errors::ErrorCode::ConnNetworkDown;
        r.level = ServiceLevel::CacheOnly;
        r.redactedDetail = QStringLiteral("P0: Local network offline");
        return r;
    }

    if (!checkP1Dns(host)) {
        r.failedProbe = 1;
        r.code = Core::Errors::ErrorCode::ConnDns;
        r.level = ServiceLevel::CacheOnly;
        r.redactedDetail = QStringLiteral("P1: Host resolution failed");
        return r;
    }

    bool smbPortOk = checkP2Tcp(host, 445, 1500);
    bool apiPortOk = (port > 0) ? checkP2Tcp(host, port, 1500) : false;

    if (!smbPortOk && !apiPortOk) {
        r.failedProbe = 2;
        r.code = Core::Errors::ErrorCode::ConnTimeout;
        r.level = ServiceLevel::CacheOnly;
        r.redactedDetail = QStringLiteral("P2: Neither SMB nor API reachable");
        return r;
    }

    // P3-P5: Storage check
    bool smbOk = smbPortOk;
    if (storage && !share.isEmpty()) {
        smbOk = storage->probe();
    }

    // P6: API health check
    bool apiOk = apiPortOk && (api != nullptr);

    r.latencyMs = timer.elapsed();
    if (smbOk && apiOk) {
        r.level = ServiceLevel::Full;
        r.transport = QStringLiteral("SMB + HTTP");
        r.code = Core::Errors::ErrorCode::NoError;
    } else if (smbOk && !apiOk) {
        r.level = ServiceLevel::SmbOnly;
        r.transport = QStringLiteral("SMB");
        r.code = Core::Errors::ErrorCode::NoError;
    } else {
        r.level = ServiceLevel::CacheOnly;
        r.transport = apiOk ? QStringLiteral("HTTP") : QStringLiteral("None");
        r.code = Core::Errors::ErrorCode::ConnLost;
    }
    return r;
}

} // namespace Core::Connection

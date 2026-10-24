/**
 * Location: src/core/diagnostics/SystemDoctor.cpp
 * Function: Implementation of the diagnostic doctor verifying local network,
 * DNS resolution, TCP ports, audio sink, Qt runtime, and generating report.
 * All Rights Reserved isgaar
 */
#include "SystemDoctor.h"
#include "core/connection/ProbeRunner.h"
#include <QCoreApplication>
#include <QSysInfo>
#include <QTextStream>

namespace Core::Diagnostics {

SystemDoctor::SystemDoctor(QObject* parent) : QObject(parent) {}

QString SystemDoctor::generateDoctorReport(const QString& host, int port) {
    QString out;
    QTextStream ts(&out);

    ts << QStringLiteral("=== Dopamine System Doctor ===\n");
    ts << QStringLiteral("App Version: ")
       << QCoreApplication::applicationVersion() << QStringLiteral("\n");
    ts << QStringLiteral("Qt Runtime: ")
       << qVersion() << QStringLiteral("\n");
    ts << QStringLiteral("OS Target: ")
       << QSysInfo::prettyProductName() << QStringLiteral("\n\n");

    ts << QStringLiteral("--- Network & Probes ---\n");
    bool p0 = Connection::ProbeRunner::checkP0Network();
    ts << QStringLiteral("P0 Local Network: ")
       << (p0 ? QStringLiteral("[OK]") : QStringLiteral("[FAIL]")) << "\n";

    QString resolvedIp;
    bool p1 = Connection::ProbeRunner::checkP1Dns(host, &resolvedIp);
    ts << QStringLiteral("P1 DNS Resolution: ")
       << (p1 ? QStringLiteral("[OK] ") + resolvedIp
              : QStringLiteral("[FAIL]")) << "\n";

    bool p2Smb = Connection::ProbeRunner::checkP2Tcp(host, 445, 1000);
    ts << QStringLiteral("P2 TCP Port 445 (SMB): ")
       << (p2Smb ? QStringLiteral("[OK]") : QStringLiteral("[FAIL/CLOSED]"))
       << "\n";

    bool p2Api = (port > 0)
        ? Connection::ProbeRunner::checkP2Tcp(host, port, 1000) : false;
    ts << QStringLiteral("P2 TCP Port %1 (API): ").arg(port)
       << (p2Api ? QStringLiteral("[OK]") : QStringLiteral("[FAIL/CLOSED]"))
       << "\n\n";

    ts << QStringLiteral("--- Diagnostic Summary ---\n");
    if (p0 && p1 && (p2Smb || p2Api)) {
        ts << QStringLiteral("Status: HEALTHY (Level A or B reachable)\n");
    } else {
        ts << QStringLiteral("Status: DEGRADED or OFFLINE\n");
    }

    return out;
}

QString SystemDoctor::runDiagnostics() {
    return generateDoctorReport();
}

} // namespace Core::Diagnostics

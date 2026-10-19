/**
 * Location: src/core/diagnostics/SystemDoctor.h
 * Function: Diagnostic inspector running P0-P7 probes, audio device checks,
 * credentials, and Qt environment reports for the CLI --doctor option.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_DIAGNOSTICS_SYSTEMDOCTOR_H
#define DOPAMINE_CORE_DIAGNOSTICS_SYSTEMDOCTOR_H

#include <QObject>
#include <QString>
#include "core/connection/ConnectionReport.h"

namespace Core::Diagnostics {

class SystemDoctor : public QObject {
    Q_OBJECT
public:
    explicit SystemDoctor(QObject* parent = nullptr);

    static QString generateDoctorReport(
        const QString& host = QStringLiteral("127.0.0.1"),
        int port = 8000
    );

    Q_INVOKABLE QString runDiagnostics();
};

} // namespace Core::Diagnostics

#endif // DOPAMINE_CORE_DIAGNOSTICS_SYSTEMDOCTOR_H

/**
 * Location: tests/test_error_system.cpp
 * Function: Unit tests verifying ErrorCode string tokens, security redaction,
 * structured JSON logger rotation, UI stall watchdog, and system doctor.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include "core/errors/ErrorCode.h"
#include "core/errors/Redactor.h"
#include "core/logging/JsonLogger.h"
#include "core/platform/UiWatchdog.h"
#include "core/diagnostics/SystemDoctor.h"

using namespace Core::Errors;
using namespace Core::Logging;
using namespace Core::Platform;
using namespace Core::Diagnostics;

class TestErrorSystem : public QObject {
    Q_OBJECT
private slots:
    void test_error_code_string_mapping() {
        QCOMPARE(errorCodeToString(ErrorCode::NoError),
                 QStringLiteral("ERR_NONE"));
        QCOMPARE(errorCodeToString(ErrorCode::ConnTimeout),
                 QStringLiteral("ERR_CONN_TIMEOUT"));
        QCOMPARE(errorCodeToString(ErrorCode::AuthBadCredentials),
                 QStringLiteral("ERR_AUTH_BAD_CREDENTIALS"));
        QCOMPARE(errorCodeToString(ErrorCode::UiStall),
                 QStringLiteral("ERR_UI_STALL"));
    }

    void test_security_redaction_rules() {
        QString rawPass = QStringLiteral("connect password=SecretPassword123");
        QString redacted = Redactor::redact(rawPass);
        QVERIFY(!redacted.contains("SecretPassword123"));
        QVERIFY(redacted.contains("[REDACTED]"));

        QString rawSmb = QStringLiteral(
            "smb://admin:SuperSecret@192.168.1.5/Music"
        );
        QString redSmb = Redactor::redactUri(rawSmb);

        QVERIFY(!redSmb.contains("admin"));
        QVERIFY(!redSmb.contains("SuperSecret"));
        QVERIFY(redSmb.contains("[USER]"));
    }

    void test_json_logger_structured_format() {
        JsonLogger::instance().log(
            LogLevel::Error,
            ErrorCode::ConnLost,
            QStringLiteral("AudioBackend"),
            QStringLiteral("stream_chunk"),
            120,
            QStringLiteral("req-abc"),
            QStringLiteral("password=LeakedPass")
        );
        QString line = JsonLogger::instance().lastLogLine();
        QVERIFY(line.contains(QStringLiteral("ERR_CONN_LOST")));
        QVERIFY(line.contains(QStringLiteral("AudioBackend")));
        QVERIFY(!line.contains(QStringLiteral("LeakedPass")));
        QVERIFY(line.contains(QStringLiteral("[REDACTED]")));
    }

    void test_ui_watchdog_stall_detection() {
        UiWatchdog watchdog;
        bool stallEmitted = false;
        connect(&watchdog, &UiWatchdog::stallDetected,
                [&](qint64, const QString&) { stallEmitted = true; });

        watchdog.start(20, 50); // 50ms threshold
        QThread::msleep(80);    // Cause stall
        QCoreApplication::processEvents();

        QVERIFY(stallEmitted);
        watchdog.stop();
    }

    void test_system_doctor_redacted_report() {
        QString report = SystemDoctor::generateDoctorReport();
        QVERIFY(report.contains(QStringLiteral("Dopamine System Doctor")));
        QVERIFY(report.contains(QStringLiteral("P0 Local Network")));
        QVERIFY(!report.contains(QStringLiteral("password")));
    }
};

QTEST_MAIN(TestErrorSystem)
#include "test_error_system.moc"

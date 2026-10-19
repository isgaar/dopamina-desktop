/**
 * Location: tests/test_ambiguity_daemon.cpp
 * Function: Unit tests for AmbiguityDaemon scheduling and supervisor restarts.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include <QSignalSpy>
#include "../src/core/daemon/AmbiguityDaemon.h"
#include "../src/core/daemon/AmbiguitySupervisor.h"

using namespace Dopamine::Core::Daemon;

class TestAmbiguityDaemon : public QObject {
    Q_OBJECT
private slots:
    void testSupervisorRestartLimits() {
        AmbiguitySupervisor sup;
        QSignalSpy spy(&sup, &AmbiguitySupervisor::fatalFailureOccurred);

        qint64 t = 1000;
        QVERIFY(sup.recordCrashAndShouldRestart(t));
        QVERIFY(sup.recordCrashAndShouldRestart(t + 10));
        QVERIFY(sup.recordCrashAndShouldRestart(t + 20));
        QCOMPARE(sup.restartCount(), 3);

        // 4th crash in window must fail
        QVERIFY(!sup.recordCrashAndShouldRestart(t + 30));
        QCOMPARE(spy.count(), 1);

        // Crash after window (1000 + 601 = 1601) should prune old
        QVERIFY(sup.recordCrashAndShouldRestart(t + 601));
    }

    void testDaemonPauseAndPriorities() {
        AmbiguityDaemon daemon;
        QSignalSpy pauseSpy(&daemon, &AmbiguityDaemon::daemonPaused);

        QVERIFY(!daemon.isPaused());
        daemon.setPlaybackRisk(true);
        QVERIFY(daemon.isPaused());
        QCOMPARE(pauseSpy.count(), 1);

        daemon.setPlaybackRisk(false);
        QVERIFY(!daemon.isPaused());
        QCOMPARE(pauseSpy.count(), 2);

        daemon.setOffline(true);
        QVERIFY(daemon.isPaused());

        daemon.enqueue(101, TaskPriority::P3_Library);
        daemon.enqueue(102, TaskPriority::P0_Seed);
        daemon.enqueue(103, TaskPriority::P1_Queue);
        QCOMPARE(daemon.pendingCount(), 3);
    }
};

QTEST_MAIN(TestAmbiguityDaemon)
#include "test_ambiguity_daemon.moc"

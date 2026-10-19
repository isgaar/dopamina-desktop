/**
 * Location: tests/test_transition_planner.cpp
 * Function: Unit test suite verifying TransitionPlanner beatmatch bar frames,
 * harmonic key compatibility, and fallback equal-power soft crossfades.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include "core/audio/automix/TransitionPlanner.h"

using namespace Dopamine::Core::Audio::Automix;

class TestTransitionPlanner : public QObject {
    Q_OBJECT

private slots:
    void testBeatmatchSameTempoAndKey() {
        auto plan = TransitionPlanner::plan(
            128.0, 0.9, QStringLiteral("8A"),
            128.0, 0.85, QStringLiteral("8A"),
            4.0, 44100);
        QCOMPARE(plan.kind, TransitionKind::Beatmatch);
        QCOMPARE(plan.bars, 16);
        qint64 expected = std::round(16 * 4.0 * 60.0 * 44100 / 128.0);
        QCOMPARE(plan.fadeFrames, expected);
    }

    void testBeatmatchFoldedOctave() {
        auto plan = TransitionPlanner::plan(
            140.0, 0.8, QStringLiteral("11B"),
            70.0, 0.75, QStringLiteral("11A"),
            4.0, 44100);
        QCOMPARE(plan.kind, TransitionKind::Beatmatch);
        QVERIFY(plan.bars == 8 || plan.bars == 16);
    }

    void testHarmonicCompatibleKey() {
        auto plan = TransitionPlanner::plan(
            125.0, 0.9, QStringLiteral("4A"),
            90.0, 0.8, QStringLiteral("5A"),
            6.0, 44100);
        QCOMPARE(plan.kind, TransitionKind::Armonico);
        QCOMPARE(plan.bars, 0);
        QCOMPARE(plan.durationSec, 6.0);
    }

    void testSoftCrossfadeFallback() {
        auto plan = TransitionPlanner::plan(
            120.0, 0.9, QStringLiteral("1A"),
            100.0, 0.9, QStringLiteral("7B"),
            3.0, 44100);
        QCOMPARE(plan.kind, TransitionKind::FundidoSuave);
        QCOMPARE(plan.durationSec, 3.0);
    }

    void testGaplessZeroDuration() {
        auto plan = TransitionPlanner::plan(
            120.0, 0.2, QStringLiteral("1A"),
            120.0, 0.2, QStringLiteral("7B"),
            0.0, 44100);
        QCOMPARE(plan.kind, TransitionKind::FundidoSuave);
        QCOMPARE(plan.durationSec, 0.0);
        QCOMPARE(plan.fadeFrames, 0);
    }
};

QTEST_MAIN(TestTransitionPlanner)
#include "test_transition_planner.moc"

/**
 * Location: tests/test_audio_dsp.cpp
 * Function: Test suite validating float32 gain scaling, null tests, 0 dB
 * precision, equal-power crossfade power sum, and BS.1770 normalization.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include <cmath>
#include <vector>
#include "core/audio/dsp/LoudnessGain.h"
#include "core/audio/dsp/GainStage.h"
#include "core/audio/automix/CrossfadeMixer.h"

using namespace Dopamine::Core::Audio::Dsp;

class TestAudioDsp : public QObject {
    Q_OBJECT
private slots:
    void testMissingLufsYieldsUnity() {
        auto res = LoudnessGain::calculate(-14.0f, std::nullopt);
        QCOMPARE(res.gainDb, 0.0f);
        QCOMPARE(res.linearGain, 1.0f);
    }

    void testAttenuationWhenLouder() {
        auto res = LoudnessGain::calculate(-14.0f, -10.0f, -1.0f);
        QCOMPARE(res.gainDb, -4.0f);
        float expected = std::pow(10.0f, -4.0f / 20.0f);
        QVERIFY(std::abs(res.linearGain - expected) < 1e-5f);
    }

    void testBoostWithPeakCap() {
        auto res = LoudnessGain::calculate(-14.0f, -20.0f, -2.0f);
        QCOMPARE(res.gainDb, 1.0f);
        float expected = std::pow(10.0f, 1.0f / 20.0f);
        QVERIFY(std::abs(res.linearGain - expected) < 1e-5f);
    }

    void testZeroDbBitExact() {
        GainStage stage;
        stage.setGain(1.0f);
        std::vector<float> samples = {0.123f, -0.456f, 0.789f, -0.999f};
        std::vector<float> copy = samples;
        stage.process(samples.data(), 2, 2);
        for (size_t i = 0; i < samples.size(); ++i) {
            QCOMPARE(samples[i], copy[i]);
        }
    }

    void testNullTestResidualBelowNegative120Db() {
        GainStage boostStage;
        boostStage.setGain(2.0f); // +6.02 dB
        GainStage cutStage;
        cutStage.setGain(0.5f);   // -6.02 dB
        std::vector<float> original = {0.25f, -0.5f, 0.75f, -0.1f};
        std::vector<float> processed = original;
        boostStage.process(processed.data(), 2, 2);
        cutStage.process(processed.data(), 2, 2);
        for (size_t i = 0; i < original.size(); ++i) {
            float residual = std::abs(processed[i] - original[i]);
            // -120 dBFS is 1e-6 amplitude
            QVERIFY(residual < 1e-6f);
        }
    }

    void testCrossfadeEqualPowerSumWithinZeroPointZeroOneDb() {
        for (int step = 0; step <= 100; ++step) {
            float progress = step / 100.0f;
            float gOut = 0.0f;
            float gIn = 0.0f;
            CrossfadeMixer::computeGains(progress, gOut, gIn);
            float powerSum = gOut * gOut + gIn * gIn;
            float powerSumDb = 10.0f * std::log10(powerSum);
            QVERIFY(std::abs(powerSumDb) < 0.01f);
        }
    }

    void testGaplessSampleContinuity() {
        CrossfadeMixer mixer;
        std::vector<float> out(4, 0.0f);
        std::vector<float> in(4, 0.5f);
        std::vector<float> mixed(4, 0.0f);
        // Step from 0 to 0 (pure out)
        mixer.mix(out.data(), in.data(), mixed.data(), 2, 2, 0.0f, 0.0f);
        QCOMPARE(mixed[0], 0.0f);
        // Step at 1 (pure in)
        mixer.mix(out.data(), in.data(), mixed.data(), 2, 2, 1.0f, 1.0f);
        QCOMPARE(mixed[0], 0.5f);
    }
};

QTEST_MAIN(TestAudioDsp)
#include "test_audio_dsp.moc"

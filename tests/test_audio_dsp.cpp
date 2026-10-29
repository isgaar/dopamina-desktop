/**
 * Location: tests/test_audio_dsp.cpp
 * Function: Test suite validating float32 gain scaling, null tests, 0 dB
 * precision, equal-power crossfade power sum, and BS.1770 normalization.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include <cmath>
#include <limits>
#include <vector>
#include "core/audio/PcmConversion.h"
#include "core/audio/dsp/EqSpec.h"
#include "core/audio/dsp/LoudnessGain.h"
#include "core/audio/dsp/GainStage.h"
#include "core/audio/automix/CrossfadeMixer.h"

using namespace Dopamine::Core::Audio::Dsp;
using namespace Dopamine::Core::Audio::Pcm;

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

    void testPcmRoundTrip_data() {
        QTest::addColumn<int>("format");
        QTest::addColumn<int>("bytesPerSample");
        QTest::addColumn<float>("tolerance");
        QTest::newRow("uint8") << int(QAudioFormat::UInt8) << 1 << 0.008f;
        QTest::newRow("int16") << int(QAudioFormat::Int16) << 2 << 1e-4f;
        QTest::newRow("int32") << int(QAudioFormat::Int32) << 4 << 1e-7f;
        QTest::newRow("float") << int(QAudioFormat::Float) << 4 << 0.0f;
    }

    void testPcmRoundTrip() {
        QFETCH(int, format);
        QFETCH(int, bytesPerSample);
        QFETCH(float, tolerance);
        const auto sampleFormat =
            static_cast<QAudioFormat::SampleFormat>(format);
        const std::vector<float> input = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
        std::vector<char> buffer(input.size() * bytesPerSample);
        for (size_t i = 0; i < input.size(); ++i) {
            encodeSample(buffer.data(), sampleFormat, int(i), input[i]);
        }
        for (size_t i = 0; i < input.size(); ++i) {
            const float decoded =
                decodeSample(buffer.data(), sampleFormat, int(i));
            QVERIFY2(std::abs(decoded - input[i]) <= tolerance,
                     qPrintable(QString("sample %1: %2 != %3")
                                    .arg(i)
                                    .arg(decoded)
                                    .arg(input[i])));
        }
    }

    void testPcmKnownValuesAndClamping() {
        const quint8 silence = 128;
        QCOMPARE(decodeSample(reinterpret_cast<const char *>(&silence),
                              QAudioFormat::UInt8, 0), 0.0f);

        const qint16 minimum = std::numeric_limits<qint16>::min();
        QCOMPARE(decodeSample(reinterpret_cast<const char *>(&minimum),
                              QAudioFormat::Int16, 0), -1.0f);

        qint16 encoded = 0;
        encodeSample(reinterpret_cast<char *>(&encoded),
                     QAudioFormat::Int16, 0, 2.0f);
        QCOMPARE(encoded, std::numeric_limits<qint16>::max());
        QCOMPARE(decodeSample(reinterpret_cast<const char *>(&encoded),
                              QAudioFormat::Int16, 0) > 0.999f, true);
    }

    void testRust12BandEqFlatZeroSoundLossAndCrossfadeOverlap() {
        EqualizerFilter eq;
        std::vector<float> flatGains(kEqBands, 0.0f);
        eq.setup(flatGains, 0.0f, 48000.0f);
        QVERIFY(eq.isFlat());

        const int frames = 1024;
        std::vector<float> orig(frames * 2);
        for (int i = 0; i < frames; ++i) {
            float s = 0.95f * std::sin(2.0f * 3.14159265f * 1000.0f * i / 48000.0f);
            orig[i * 2] = s;
            orig[i * 2 + 1] = -s;
        }
        std::vector<float> processed = orig;
        eq.process(processed.data(), frames, 2);
        for (size_t i = 0; i < orig.size(); ++i) {
            QVERIFY(std::abs(processed[i] - orig[i]) < 1e-6f);
        }

        // Verify Rust tail-buffer crossfade blends Track A into Track B
        std::vector<float> trackA(512 * 2, 0.8f);
        eq.process(trackA.data(), 512, 2);
        dopamine_dsp_trigger_crossfade(256);
        std::vector<float> trackB(256 * 2, 0.4f);
        eq.process(trackB.data(), 256, 2);
        QVERIFY( std::abs(trackB[0] - 0.8f) < 0.03f );
        QVERIFY( std::abs(trackB[(255 * 2)] - 0.4f) < 0.03f );
        eq.resetStates();
    }
};

QTEST_MAIN(TestAudioDsp)
#include "test_audio_dsp.moc"

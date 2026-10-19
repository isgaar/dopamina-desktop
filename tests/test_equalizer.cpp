/**
 * Location: tests/test_equalizer.cpp
 * Function: Test suite for T-EQ-1..6 validating 12-band equalizer defaults,
 * reset idempotency, corrupted settings recovery, and bit-exact null tests.
 * All Rights Reserved isgaar
 */
#include <QTest>
#include <QSettings>
#include <vector>
#include <cmath>
#include "core/audio/dsp/EqSpec.h"
#include "features/playback/app/EqualizerController.h"

using namespace Dopamine::Core::Audio::Dsp;
using namespace Dopamine::Features::Playback::App;

class TestEqualizer : public QObject {
    Q_OBJECT
private slots:
    void init() {
        QSettings("isgaar", "Dopamine").remove("Equalizer");
    }

    void cleanup() {
        QSettings("isgaar", "Dopamine").remove("Equalizer");
    }

    void test_TEQ1_default_state_is_flat() {
        EqualizerController ctrl;
        QCOMPARE(ctrl.preamp(), kEqFlatDb);
        QVariantList bg = ctrl.bandGains();
        QCOMPARE(bg.size(), kEqBands);
        for (int i = 0; i < kEqBands; ++i) {
            QCOMPARE(bg[i].toDouble(), kEqFlatDb);
        }
    }

    void test_TEQ2_randomize_then_reset_yields_zero() {
        EqualizerController ctrl;
        ctrl.setPreamp(6.5);
        for (int i = 0; i < kEqBands; ++i) {
            ctrl.setBandGain(i, (i % 2 == 0) ? 3.0 : -4.0);
        }
        ctrl.reset();
        QCOMPARE(ctrl.preamp(), kEqFlatDb);
        for (int i = 0; i < kEqBands; ++i) {
            QCOMPARE(ctrl.bandGains()[i].toDouble(), kEqFlatDb);
        }
    }

    void test_TEQ3_corrupt_settings_recovery() {
        {
            QSettings s("isgaar", "Dopamine");
            s.setValue("Equalizer/userMoved", true);
            s.setValue("Equalizer/preamp", 999.0);
            QVariantList bad;
            bad.append(0.0);
            s.setValue("Equalizer/bandGains", bad);
        }
        EqualizerController ctrl;
        QCOMPARE(ctrl.preamp(), kEqFlatDb);
        QCOMPARE(ctrl.bandGains().size(), kEqBands);
        for (int i = 0; i < kEqBands; ++i) {
            QCOMPARE(ctrl.bandGains()[i].toDouble(), kEqFlatDb);
        }
    }

    void test_TEQ4_flat_eq_is_bit_exact_and_non_flat_modifies() {
        EqualizerFilter filter;
        std::vector<float> flatGains(kEqBands, 0.0f);
        filter.setup(flatGains, 0.0f, 44100.0f);
        QVERIFY(filter.isFlat());

        std::vector<float> original = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f};
        std::vector<float> data = original;
        filter.process(data.data(), static_cast<int>(data.size()), 1);
        for (size_t i = 0; i < original.size(); ++i) {
            QCOMPARE(data[i], original[i]);
        }

        std::vector<float> boostGains(kEqBands, 6.0f);
        filter.setup(boostGains, 0.0f, 44100.0f);
        QVERIFY(!filter.isFlat());
        filter.process(data.data(), static_cast<int>(data.size()), 1);
        bool modified = false;
        for (size_t i = 0; i < original.size(); ++i) {
            if (std::abs(data[i] - original[i]) > 1e-5f) modified = true;
        }
        QVERIFY(modified);
    }

    void test_TEQ5_reset_twice_is_idempotent() {
        EqualizerController ctrl;
        ctrl.setPreamp(5.0);
        ctrl.reset();
        auto firstGains = ctrl.bandGains();
        auto firstPreamp = ctrl.preamp();
        ctrl.reset();
        QCOMPARE(ctrl.preamp(), firstPreamp);
        QCOMPARE(ctrl.bandGains(), firstGains);
    }

    void test_TEQ6_flat_preset_matches_reset_vector() {
        EqualizerController ctrl;
        ctrl.reset();
        auto resetGains = ctrl.bandGains();
        auto resetPreamp = ctrl.preamp();
        ctrl.loadPreset("Flat");
        QCOMPARE(ctrl.preamp(), resetPreamp);
        QCOMPARE(ctrl.bandGains(), resetGains);
    }
};

QTEST_MAIN(TestEqualizer)
#include "test_equalizer.moc"

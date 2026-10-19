/**
 * Location: src/features/playback/domain/EqualizerPreset.h
 * Function: Stellio-grade 12-band frequency constants, acoustic presets,
 * DSP sound effects parameters, and gain limit definitions for equalizer.
 * All Rights Reserved isgaar
 */
#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include "core/audio/dsp/EqSpec.h"

namespace Dopamine::Features::Playback::Domain {

inline QList<double> getEqualizerFrequencies() {
    QList<double> list;
    for (double f : Core::Audio::Dsp::kEqCenterHz) list.append(f);
    return list;
}

inline const QList<double> EQUALIZER_FREQUENCIES = getEqualizerFrequencies();

inline const QStringList EQUALIZER_BAND_LABELS = {
    "30 Hz", "50 Hz", "90 Hz", "160 Hz", "300 Hz", "500 Hz",
    "1 kHz", "1.6 kHz", "3 kHz", "5 kHz", "9 kHz", "16 kHz"
};

struct EqualizerPreset {
    QString name;
    double preampDb = Core::Audio::Dsp::kEqFlatDb;
    QList<double> gains = QList<double>(Core::Audio::Dsp::kEqBands,
                                        Core::Audio::Dsp::kEqFlatDb);
    bool limiter = false;
    bool agc = false;
    double balance = 0.0;
    double zBass = 0.0;
    double compressor = 0.0;
    double zMids = 0.0;
    double zTreble = 0.0;
    double pitch = 0.0;
    double speed = 1.0;
    double echo = 0.0;
    double autoWah = 0.0;
    double reverb = 0.0;
    bool flanger = false;
};

inline QMap<QString, EqualizerPreset> getStandardPresets() {
    return {
        {"Flat", {"Flat", Core::Audio::Dsp::kEqFlatDb,
         QList<double>(Core::Audio::Dsp::kEqBands, Core::Audio::Dsp::kEqFlatDb),
         false, false, 0.0,
         0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, false}},
        {"Rock", {"Rock", 1.0,
         {4.5,3.0,1.5,-0.5,-1.5,-0.5,1.5,3.0,4.0,4.5,4.0,3.5},
         true, false, 0.0, 20.0, 15.0, 0.0, 10.0,
         0.0, 1.0, 0.0, 0.0, 0.0, false}},
        {"Pop", {"Pop", 0.5,
         {-1.5,1.0,3.0,3.5,2.0,0.0,-1.0,-1.5,1.0,2.5,3.0,3.5},
         false, false, 0.0, 10.0, 0.0, 15.0, 15.0,
         0.0, 1.0, 0.0, 0.0, 0.0, false}},
        {"Classical", {"Classical", 0.0,
         {3.5,3.0,2.5,1.5,0.0,-1.5,-2.0,0.5,2.0,3.0,3.5,4.0},
         false, false, 0.0, 0.0, 0.0, 0.0, 10.0,
         0.0, 1.0, 0.0, 0.0, 15.0, false}},
        {"Dance", {"Dance", 1.5,
         {6.0,5.0,2.5,0.0,-1.0,0.0,2.0,3.5,4.5,5.0,5.5,4.5},
         true, false, 0.0, 35.0, 25.0, 0.0, 25.0,
         0.0, 1.0, 0.0, 0.0, 0.0, false}},
        {"Electro", {"Electro", 1.5,
         {5.5,4.5,2.0,-0.5,-1.5,0.5,2.5,4.0,5.0,5.5,6.0,5.0},
         true, false, 0.0, 30.0, 28.0, 0.0, 30.0,
         0.0, 1.0, 0.0, 20.0, 0.0, false}},
        {"Bass Booster", {"Bass Booster", 1.0,
         {7.0,6.0,4.5,2.5,1.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},
         true, false, 0.0, 50.0, 0.0, 0.0, 0.0,
         0.0, 1.0, 0.0, 0.0, 0.0, false}},
        {"Vocal", {"Vocal", 0.0,
         {-2.0,-1.5,-0.5,1.5,3.5,4.0,3.5,2.0,0.5,-1.0,-1.5,-2.0},
         false, false, 0.0, 0.0, 0.0, 25.0, 10.0,
         0.0, 1.0, 0.0, 0.0, 0.0, false}}
    };
}

} // namespace Dopamine::Features::Playback::Domain

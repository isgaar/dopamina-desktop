/**
 * Location: src/features/playback/app/EqualizerController.cpp
 * Function: Implementation of Stellio EqualizerController managing 12-band
 * gains, preamp, and 10 studio DSP audio effects with preset persistence.
 * All Rights Reserved isgaar
 */
#include <QSettings>
#include "features/playback/app/EqualizerController.h"

namespace Dopamine::Features::Playback::App {

EqualizerController::EqualizerController(QObject *parent)
    : QObject(parent), m_state() { loadSettings(); }

#define EQ_SET(method, call) \
    void EqualizerController::method { if (m_state.call) notifyChanges(); }

EQ_SET(setPreamp(double db), setPreamp(db))
EQ_SET(setLimiter(bool en), setLimiter(en))
EQ_SET(setAgc(bool en), setAgc(en))
EQ_SET(setBalance(double v), setBalance(v))
EQ_SET(setZBass(double v), setZBass(v))
EQ_SET(setZBassActive(bool en), setZBassActive(en))
EQ_SET(setCompressor(double v), setCompressor(v))
EQ_SET(setCompressorActive(bool en), setCompressorActive(en))
EQ_SET(setZMids(double v), setZMids(v))
EQ_SET(setZMidsActive(bool en), setZMidsActive(en))
EQ_SET(setZTreble(double v), setZTreble(v))
EQ_SET(setZTrebleActive(bool en), setZTrebleActive(en))
EQ_SET(setPitch(double v), setPitch(v))
EQ_SET(setSpeed(double v), setSpeed(v))
EQ_SET(setEcho(double v), setEcho(v))
EQ_SET(setEchoActive(bool en), setEchoActive(en))
EQ_SET(setAutoWah(double v), setAutoWah(v))
EQ_SET(setAutoWahActive(bool en), setAutoWahActive(en))
EQ_SET(setReverb(double v), setReverb(v))
EQ_SET(setReverbActive(bool en), setReverbActive(en))
EQ_SET(setFlanger(bool en), setFlanger(en))
EQ_SET(setBandGain(int idx, double g), setBandGain(idx, g))
EQ_SET(loadPreset(const QString &n), loadPreset(n))
void EqualizerController::savePreset(const QString &n) {
    if (m_state.savePreset(n)) { emit presetsChanged(); notifyChanges(); } }

void EqualizerController::resetBands() {
    m_state.resetBands(); m_userMoved = false;
    QSettings("isgaar", "Dopamine").remove("Equalizer");
    emit paramsChanged(); emit presetChanged();
}
void EqualizerController::resetEffects1() {
    m_state.resetEffects1(); notifyChanges(); }
void EqualizerController::resetEffects2() {
    m_state.resetEffects2(); notifyChanges(); }
void EqualizerController::reset() {
    m_state.reset(); m_userMoved = false;
    QSettings("isgaar", "Dopamine").remove("Equalizer");
    emit paramsChanged(); emit presetChanged();
}

void EqualizerController::setEffect1Knob(int i, double v) {
    if (i == 0) setZBass(v); else if (i == 1) setCompressor(v);
    else if (i == 2) setZMids(v); else if (i == 3) setZTreble(v);
}
void EqualizerController::toggleEffect1Knob(int i, bool en) {
    if (i == 0) setZBassActive(en); else if (i == 1) setCompressorActive(en);
    else if (i == 2) setZMidsActive(en); else if (i == 3) setZTrebleActive(en);
}
void EqualizerController::setEffect2Knob(int i, double v) {
    if (i == 0) setEcho(v); else if (i == 1) setAutoWah(v);
    else if (i == 2) setReverb(v);
}
void EqualizerController::toggleEffect2Knob(int i, bool en) {
    if (i == 0) setEchoActive(en); else if (i == 1) setAutoWahActive(en);
    else if (i == 2) setReverbActive(en);
}
void EqualizerController::loadSettings() {
    QSettings s("isgaar", "Dopamine");
    m_userMoved = s.value("Equalizer/userMoved", false).toBool();
    if (!m_userMoved) { m_state.reset(); return; }
    double pre = s.value("Equalizer/preamp", 0.0).toDouble();
    QVariantList bg = s.value("Equalizer/bandGains").toList();
    bool ok = (bg.size() == Core::Audio::Dsp::kEqBands) &&
              std::isfinite(pre) && (pre >= Core::Audio::Dsp::kEqGainMinDb) &&
              (pre <= Core::Audio::Dsp::kEqGainMaxDb);
    if (ok) {
        for (const auto &v : bg) {
            double d = v.toDouble();
            if (!std::isfinite(d) || d < Core::Audio::Dsp::kEqGainMinDb ||
                d > Core::Audio::Dsp::kEqGainMaxDb) { ok = false; break; }
        }
    }
    if (!ok) { reset(); return; }
    m_state.loadPreset(s.value("Equalizer/preset", "Flat").toString());
    m_state.setPreamp(pre);
    for (int i = 0; i < bg.size(); ++i)
        m_state.setBandGain(i, bg[i].toDouble());
}
void EqualizerController::saveSettings() {
    QSettings s("isgaar", "Dopamine");
    s.setValue("Equalizer/userMoved", m_userMoved);
    if (!m_userMoved) return;
    s.setValue("Equalizer/preset", m_state.currentPreset());
    s.setValue("Equalizer/preamp", m_state.preamp());
    s.setValue("Equalizer/bandGains", m_state.bandGains());
}
void EqualizerController::notifyChanges() {
    m_userMoved = true; saveSettings();
    emit paramsChanged(); emit presetChanged();
}

} // namespace Dopamine::Features::Playback::App

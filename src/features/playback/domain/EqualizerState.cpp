/**
 * Location: src/features/playback/domain/EqualizerState.cpp
 * Function: Implementation of Stellio equalizer domain state managing gain
 * limits, audio alterations, preset catalog manipulation, and resets.
 * All Rights Reserved isgaar
 */
#include "features/playback/domain/EqualizerState.h"

namespace Dopamine::Features::Playback::Domain {

EqualizerState::EqualizerState() : m_presets(getStandardPresets()) {
    loadPreset("Flat");
}

QStringList EqualizerState::presetNames() const {
    const QStringList order = {"Flat", "Rock", "Pop", "Classical",
                               "Dance", "Electro", "Bass Booster", "Vocal"};
    QStringList res;
    for (const auto &n : order) {
        if (m_presets.contains(n)) res.append(n);
    }
    for (const auto &k : m_presets.keys()) {
        if (!res.contains(k)) res.append(k);
    }
    return res;
}

QVariantList EqualizerState::bandGains() const {
    QVariantList list;
    for (double g : m_cur.gains) list.append(g);
    return list;
}

bool EqualizerState::setBandGain(int idx, double gainDb) {
    if (idx < 0 || idx >= m_cur.gains.size()) return false;
    m_cur.gains[idx] = std::clamp(gainDb,
                                  Core::Audio::Dsp::kEqGainMinDb,
                                  Core::Audio::Dsp::kEqGainMaxDb);
    markCustom();
    return true;
}

void EqualizerState::markCustom() {
    m_currentPreset = "No guardado";
}

bool EqualizerState::loadPreset(const QString &n) {
    if (!m_presets.contains(n)) return false;
    m_cur = m_presets.value(n);
    m_zBassActive = m_cur.zBass > 0.0;
    m_compressorActive = m_cur.compressor > 0.0;
    m_zMidsActive = m_cur.zMids > 0.0;
    m_zTrebleActive = m_cur.zTreble > 0.0;
    m_echoActive = m_cur.echo > 0.0;
    m_autoWahActive = m_cur.autoWah > 0.0;
    m_reverbActive = m_cur.reverb > 0.0;
    m_currentPreset = n;
    return true;
}

bool EqualizerState::savePreset(const QString &n) {
    if (n.trimmed().isEmpty()) return false;
    m_cur.name = n.trimmed();
    m_presets[m_cur.name] = m_cur;
    m_currentPreset = m_cur.name;
    return true;
}

void EqualizerState::resetBands() {
    m_cur.preampDb = Core::Audio::Dsp::kEqFlatDb; m_cur.limiter = false;
    m_cur.gains = QList<double>(Core::Audio::Dsp::kEqBands,
                                Core::Audio::Dsp::kEqFlatDb);
    markCustom();
}

void EqualizerState::resetEffects1() {
    m_cur.agc = false; m_cur.balance = 0.0;
    m_cur.zBass = 0.0; m_zBassActive = false;
    m_cur.compressor = 0.0; m_compressorActive = false;
    m_cur.zMids = 0.0; m_zMidsActive = false;
    m_cur.zTreble = 0.0; m_zTrebleActive = false;
    markCustom();
}

void EqualizerState::resetEffects2() {
    m_cur.pitch = 0.0; m_cur.speed = 1.0;
    m_cur.echo = 0.0; m_echoActive = false;
    m_cur.autoWah = 0.0; m_autoWahActive = false;
    m_cur.reverb = 0.0; m_reverbActive = false;
    m_cur.flanger = false;
    markCustom();
}

void EqualizerState::reset() {
    loadPreset("Flat");
}

} // namespace Dopamine::Features::Playback::Domain

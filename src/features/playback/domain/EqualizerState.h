/**
 * Location: src/features/playback/domain/EqualizerState.h
 * Function: Domain model tracking active equalizer parameters, custom presets,
 * audio DSP effect toggle flags, and parameter clamping restrictions.
 * All Rights Reserved isgaar
 */
#pragma once

#include <algorithm>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QMap>
#include "features/playback/domain/EqualizerPreset.h"

namespace Dopamine::Features::Playback::Domain {

class EqualizerState {
public:
    EqualizerState();

    double preamp() const { return m_cur.preampDb; }
    bool limiter() const { return m_cur.limiter; }
    bool agc() const { return m_cur.agc; }
    double balance() const { return m_cur.balance; }
    double zBass() const { return m_cur.zBass; }
    bool zBassActive() const { return m_zBassActive; }
    double compressor() const { return m_cur.compressor; }
    bool compressorActive() const { return m_compressorActive; }
    double zMids() const { return m_cur.zMids; }
    bool zMidsActive() const { return m_zMidsActive; }
    double zTreble() const { return m_cur.zTreble; }
    bool zTrebleActive() const { return m_zTrebleActive; }
    double pitch() const { return m_cur.pitch; }
    double speed() const { return m_cur.speed; }
    double echo() const { return m_cur.echo; }
    bool echoActive() const { return m_echoActive; }
    double autoWah() const { return m_cur.autoWah; }
    bool autoWahActive() const { return m_autoWahActive; }
    double reverb() const { return m_cur.reverb; }
    bool reverbActive() const { return m_reverbActive; }
    bool flanger() const { return m_cur.flanger; }
    QStringList presetNames() const;
    QString currentPreset() const { return m_currentPreset; }
    QVariantList bandGains() const;

    bool setPreamp(double db) {
        m_cur.preampDb = std::clamp(db, -12.0, 12.0); markCustom(); return true;
    }
    bool setLimiter(bool en) { return setFlag(m_cur.limiter, en); }
    bool setAgc(bool en) { return setFlag(m_cur.agc, en); }
    bool setBalance(double v) {
        m_cur.balance = std::clamp(v, -100.0, 100.0); markCustom(); return true;
    }
    bool setZBass(double v) {
        m_cur.zBass = std::clamp(v, 0.0, 100.0); markCustom(); return true;
    }
    bool setZBassActive(bool en) { return setFlag(m_zBassActive, en); }
    bool setCompressor(double v) {
        m_cur.compressor = std::clamp(v, 0.0, 100.0); markCustom(); return true;
    }
    bool setCompressorActive(bool en) {
        return setFlag(m_compressorActive, en);
    }
    bool setZMids(double v) {
        m_cur.zMids = std::clamp(v, 0.0, 100.0); markCustom(); return true;
    }
    bool setZMidsActive(bool en) { return setFlag(m_zMidsActive, en); }
    bool setZTreble(double v) {
        m_cur.zTreble = std::clamp(v, 0.0, 100.0); markCustom(); return true;
    }
    bool setZTrebleActive(bool en) { return setFlag(m_zTrebleActive, en); }
    bool setPitch(double v) {
        m_cur.pitch = std::clamp(v, -12.0, 12.0); markCustom(); return true;
    }
    bool setSpeed(double v) {
        m_cur.speed = std::clamp(v, 0.3, 3.0); markCustom(); return true;
    }
    bool setEcho(double v) {
        m_cur.echo = std::clamp(v, 0.0, 100.0); markCustom(); return true;
    }
    bool setEchoActive(bool en) { return setFlag(m_echoActive, en); }
    bool setAutoWah(double v) {
        m_cur.autoWah = std::clamp(v, 0.0, 100.0); markCustom(); return true;
    }
    bool setAutoWahActive(bool en) { return setFlag(m_autoWahActive, en); }
    bool setReverb(double v) {
        m_cur.reverb = std::clamp(v, 0.0, 100.0); markCustom(); return true;
    }
    bool setReverbActive(bool en) { return setFlag(m_reverbActive, en); }
    bool setFlanger(bool en) { return setFlag(m_cur.flanger, en); }
    bool setBandGain(int idx, double gainDb);

    bool loadPreset(const QString &name);
    bool savePreset(const QString &name);
    void resetBands();
    void resetEffects1();
    void resetEffects2();
    void reset();

private:
    void markCustom();
    bool setFlag(bool &flag, bool val) {
        flag = val; markCustom(); return true;
    }
    EqualizerPreset m_cur;
    bool m_zBassActive = false, m_compressorActive = false;
    bool m_zMidsActive = false, m_zTrebleActive = false;
    bool m_echoActive = false, m_autoWahActive = false;
    bool m_reverbActive = false;
    QString m_currentPreset = "Flat";
    QMap<QString, EqualizerPreset> m_presets;
};

} // namespace Dopamine::Features::Playback::Domain

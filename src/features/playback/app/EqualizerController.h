/**
 * Location: src/features/playback/app/EqualizerController.h
 * Function: Application controller exposing Stellio-grade 12-band equalizer
 * gains, preamp, and 10 studio DSP audio effects to Qt Quick components.
 * All Rights Reserved isgaar
 */
#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include "features/playback/domain/EqualizerState.h"

namespace Dopamine::Features::Playback::App {

class EqualizerController : public QObject {
    Q_OBJECT
    Q_PROPERTY(double preamp READ preamp WRITE setPreamp NOTIFY paramsChanged)
    Q_PROPERTY(QVariantList bandGains READ bandGains NOTIFY paramsChanged)
    Q_PROPERTY(QStringList bandLabels READ bandLabels CONSTANT)
    Q_PROPERTY(bool limiter READ limiter WRITE setLimiter NOTIFY paramsChanged)
    Q_PROPERTY(bool agc READ agc WRITE setAgc NOTIFY paramsChanged)
    Q_PROPERTY(double balance READ balance WRITE setBalance
               NOTIFY paramsChanged)
    Q_PROPERTY(double zBass READ zBass WRITE setZBass NOTIFY paramsChanged)
    Q_PROPERTY(bool zBassActive READ zBassActive WRITE setZBassActive
               NOTIFY paramsChanged)
    Q_PROPERTY(double compressor READ compressor WRITE setCompressor
               NOTIFY paramsChanged)
    Q_PROPERTY(bool compressorActive READ compressorActive
               WRITE setCompressorActive NOTIFY paramsChanged)
    Q_PROPERTY(double zMids READ zMids WRITE setZMids NOTIFY paramsChanged)
    Q_PROPERTY(bool zMidsActive READ zMidsActive WRITE setZMidsActive
               NOTIFY paramsChanged)
    Q_PROPERTY(double zTreble READ zTreble WRITE setZTreble
               NOTIFY paramsChanged)
    Q_PROPERTY(bool zTrebleActive READ zTrebleActive WRITE setZTrebleActive
               NOTIFY paramsChanged)
    Q_PROPERTY(double pitch READ pitch WRITE setPitch NOTIFY paramsChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY paramsChanged)
    Q_PROPERTY(double echo READ echo WRITE setEcho NOTIFY paramsChanged)
    Q_PROPERTY(bool echoActive READ echoActive WRITE setEchoActive
               NOTIFY paramsChanged)
    Q_PROPERTY(double autoWah READ autoWah WRITE setAutoWah
               NOTIFY paramsChanged)
    Q_PROPERTY(bool autoWahActive READ autoWahActive WRITE setAutoWahActive
               NOTIFY paramsChanged)
    Q_PROPERTY(double reverb READ reverb WRITE setReverb NOTIFY paramsChanged)
    Q_PROPERTY(bool reverbActive READ reverbActive WRITE setReverbActive
               NOTIFY paramsChanged)
    Q_PROPERTY(bool flanger READ flanger WRITE setFlanger NOTIFY paramsChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetsChanged)
    Q_PROPERTY(QString currentPreset READ currentPreset NOTIFY presetChanged)
    Q_PROPERTY(bool userMoved READ userMoved NOTIFY paramsChanged)

public:
    explicit EqualizerController(QObject *parent = nullptr);

    double preamp() const { return m_state.preamp(); }
    QVariantList bandGains() const { return m_state.bandGains(); }
    QStringList bandLabels() const { return Domain::EQUALIZER_BAND_LABELS; }
    bool limiter() const { return m_state.limiter(); }
    bool agc() const { return m_state.agc(); }
    double balance() const { return m_state.balance(); }
    double zBass() const { return m_state.zBass(); }
    bool zBassActive() const { return m_state.zBassActive(); }
    double compressor() const { return m_state.compressor(); }
    bool compressorActive() const { return m_state.compressorActive(); }
    double zMids() const { return m_state.zMids(); }
    bool zMidsActive() const { return m_state.zMidsActive(); }
    double zTreble() const { return m_state.zTreble(); }
    bool zTrebleActive() const { return m_state.zTrebleActive(); }
    double pitch() const { return m_state.pitch(); }
    double speed() const { return m_state.speed(); }
    double echo() const { return m_state.echo(); }
    bool echoActive() const { return m_state.echoActive(); }
    double autoWah() const { return m_state.autoWah(); }
    bool autoWahActive() const { return m_state.autoWahActive(); }
    double reverb() const { return m_state.reverb(); }
    bool reverbActive() const { return m_state.reverbActive(); }
    bool flanger() const { return m_state.flanger(); }
    bool userMoved() const { return m_userMoved; }
    QStringList presetNames() const { return m_state.presetNames(); }
    QString currentPreset() const { return m_state.currentPreset(); }

    void setPreamp(double db);
    void setLimiter(bool en); void setAgc(bool en);
    void setBalance(double v);
    void setZBass(double v); void setZBassActive(bool en);
    void setCompressor(double v); void setCompressorActive(bool en);
    void setZMids(double v); void setZMidsActive(bool en);
    void setZTreble(double v); void setZTrebleActive(bool en);
    void setPitch(double v); void setSpeed(double v);
    void setEcho(double v); void setEchoActive(bool en);
    void setAutoWah(double v); void setAutoWahActive(bool en);
    void setReverb(double v); void setReverbActive(bool en);
    void setFlanger(bool en);

    Q_INVOKABLE void setBandGain(int idx, double gainDb);
    Q_INVOKABLE void loadPreset(const QString &n);
    Q_INVOKABLE void savePreset(const QString &n);
    Q_INVOKABLE void resetBands(); void resetEffects1();
    Q_INVOKABLE void resetEffects2(); void reset();
    Q_INVOKABLE void setEffect1Knob(int i, double v);
    Q_INVOKABLE void toggleEffect1Knob(int i, bool en);
    Q_INVOKABLE void setEffect2Knob(int i, double v);
    Q_INVOKABLE void toggleEffect2Knob(int i, bool en);

signals:
    void paramsChanged(); void presetChanged(); void presetsChanged();

private:
    void notifyChanges();
    void loadSettings(); void saveSettings();
    bool m_userMoved = false;
    Domain::EqualizerState m_state;
};

} // namespace Dopamine::Features::Playback::App
